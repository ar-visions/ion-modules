#include <web/web.hpp>
#include <netdb.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>

#if !defined(_WIN32)
#define closesocket close
typedef int SOCKET;
#endif


struct SocketInternal {
    bool                     secure;
    int64_t                  timeout_ms;
    SOCKET                   fd_insecure;
    mbedtls_x509_crt         ca_cert;
    mbedtls_net_context      ctx;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_pk_context       pkey; // for server only
};

void Socket::set_timeout(int64_t t) {
    intern->timeout_ms = t;
}

Socket::Socket(std::nullptr_t n) { }

Socket::Socket(Role role) {
    ///
    intern = std::shared_ptr<SocketInternal>(new SocketInternal);
    SocketInternal &i = *(intern.get());
    memset(&i, 0, sizeof(SocketInternal));
    intern->secure = !is_debug();

    cchar_t *pers = "ion:web";
    mbedtls_debug_set_threshold(3);
    mbedtls_net_init         (&i.ctx);
    mbedtls_ssl_init         (&i.ssl);
    mbedtls_ssl_config_init  (&i.conf);
    mbedtls_x509_crt_init    (&i.ca_cert);
    mbedtls_pk_init          (&i.pkey);
    mbedtls_ctr_drbg_init    (&i.ctr_drbg);
    mbedtls_entropy_init     (&i.entropy);
    if (mbedtls_ctr_drbg_seed(&i.ctr_drbg, mbedtls_entropy_func,
                              &i.entropy, (const unsigned char *)pers, strlen(pers)) != 0) {
        std::cerr << " failed: mbedtls_ctr_drbg_seed\n";
        exit(1);
    }
    /// [generate certs in orbit]
    Path   cert_path = (role == Server) ? "trust/server.cer" : "trust";
    auto       certs = cert_path.matching({".cer"});
    for (auto &p: certs) {
        std::string str = p;
        int r = mbedtls_x509_crt_parse_file(&i.ca_cert, str.c_str());
        if (r != 0) {
            std::cerr << "mbedtls_x509_crt_parse_file failure: " << str << std::endl;
            exit(1);
        }
    }
    if (role == Server) {
        Path key = "server.key";
        if (key.exists()) {
            console.assertion(key.exists(), "server key: {0} does not exist", { path_t(key) });
            ///
            std::string p = key;
            int r = mbedtls_pk_parse_keyfile(&i.pkey, p.c_str(),
                        nullptr, mbedtls_ctr_drbg_random, &i.ctr_drbg);
            if (r != 0) {
                std::cerr << "mbedtls_x509_crt_parse_file failure: " << r << std::endl;
                exit(1);
            }
        }
    }
    /// debug-based param
#if defined(NDEBUG)
    mbedtls_ssl_conf_authmode(&i.conf, MBEDTLS_SSL_VERIFY_REQUIRED);
#else
    mbedtls_ssl_conf_authmode(&i.conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
#endif
}

Socket &Socket::operator=(const Socket &ref) {
    if (this != &ref) {
        uri       = ref.uri;
        intern    = ref.intern;
        connected = ref.connected;
    }
    return *this;
}

Socket:: Socket(const Socket &ref) : uri(ref.uri), intern(ref.intern), connected(ref.connected) { }
Socket::~Socket() { }

int Socket::read_raw(cchar_t *v, size_t sz) {
    SocketInternal &i = *intern;
    int r = i.secure ? mbedtls_ssl_read(&i.ssl, (unsigned char *)v, sz) :
                                   recv(i.fd_insecure,   (void *)v, sz, 0);
    return r;
}

bool Socket::read(cchar_t *v, size_t sz) {
    SocketInternal &i = *intern;
    int            st = 0;
    ///
    for (int len = sz; len > 0;) {
        int r = i.secure ? mbedtls_ssl_read(&i.ssl, (unsigned char *)&v[st], len) :
                                       recv(i.fd_insecure,   (void *)&v[st], len, 0);
        if (r <= 0)
            return false;
        len -= r;
        st  += r;
    }
    return true;
}

array<char> Socket::read_until(str s, int max_len) {
    auto rbytes = array<char>(max_len);
    size_t slen = s.size();
    ///
    for (;;) {
        rbytes   += '\0';
        size_t sz = rbytes.size();
        if (!read(&rbytes[sz - 1], 1))
            return array<char> { };
        if (sz >= slen and memcmp(&rbytes[sz - slen], s.cstr(), slen) == 0)
            break;
        if (sz == max_len)
            return array<char> { };
    }
    return rbytes;
}

bool Socket::write(cchar_t *v, size_t sz, int flags) {
    SocketInternal &i = *intern;
    size_t tlen = sz;
    size_t  len = tlen;
    size_t ibuf = 0;
    assert(strlen(v) == sz);
    
    /// write some bytes
    for (;;) {
        auto wlen = i.secure ?
                    mbedtls_ssl_write(&i.ssl, (const unsigned char *)&v[ibuf], len) :
                    send(i.fd_insecure, &v[ibuf], len, 0);
        if (wlen  < 0)
            return false;
        ibuf += wlen;
        len  -= wlen;
        if (len <= 0)
            break;
        assert(ibuf <= tlen);
    }
    return true;
}

bool Socket::write(array<char> &v) {
    return write(v.data(), v.size(), 0);
}

bool Socket::write(str s, array<var> a) {
    str f = var::format(s, a);
    return write(f.cstr(), f.size(), 0);
}

bool Socket::write(var &v) {
    if (v == Type::Str)
        return write(str(v));
    assert(false); // todo
    return true;
}

void Socket::close() const {
    SocketInternal &i = *intern;
    if (i.secure) {
        mbedtls_net_close      (&i.ctx);
        mbedtls_net_free       (&i.ctx);
        mbedtls_ssl_free       (&i.ssl);
        mbedtls_ssl_config_free(&i.conf);
        mbedtls_ctr_drbg_free  (&i.ctr_drbg);
        mbedtls_entropy_free   (&i.entropy);
        mbedtls_pk_free        (&i.pkey);
    } else
        ::closesocket(i.fd_insecure);
}

void Socket::logging(void *ctx, int level, cchar_t *file, int line, cchar_t *str) {
    fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
}

Socket Socket::connect(URI uri, Trust trust_level) {
    URI    a = uri.methodize(URI::Method::Get);
    str   qy = a.query;
    Socket s = null;
    str    p = a.proto;
    str    h = a.host;
    int port = a.port;
    ///
    if (p == "https") {
        s = Socket(Client);
        SocketInternal &i = *(s.intern);
        string s_port = string(port);

        if (mbedtls_net_connect(
                &i.ctx, h.cstr(), s_port.cstr(), MBEDTLS_NET_PROTO_TCP) != 0)
            return null;
        
        if (mbedtls_ssl_config_defaults(
                &i.conf, MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
            return null;
        
        /// set verification auth mode
        mbedtls_ssl_conf_ca_chain(&i.conf, &i.ca_cert, NULL);
        mbedtls_ssl_conf_rng(&i.conf, mbedtls_ctr_drbg_random, &i.ctr_drbg);
        mbedtls_ssl_conf_dbg(&i.conf, logging, stdout);
        
        /// bind who we are; this will be tied into keys associated at installation time of station
        /// simply to be made at that point, and instructions given for storage
        /// (outside of repo, or git-ignored)
        //if (mbedtls_ssl_conf_own_cert(&i.conf, &i.ca_cert, &i.pkey) != 0)
        //    return null;
        
        if (mbedtls_ssl_setup(&i.ssl, &i.conf) != 0)
            return null;

        if (mbedtls_ssl_set_hostname(&i.ssl, h.cstr()) != 0)
            return null;

        mbedtls_ssl_set_bio(&i.ssl, &i.ctx, mbedtls_net_send, mbedtls_net_recv, NULL);
        
        /// handshake
        for (;;) {
            int hs = mbedtls_ssl_handshake(&i.ssl);
            if (hs == 0)
                break;
            
            if (hs != MBEDTLS_ERR_SSL_WANT_READ && hs != MBEDTLS_ERR_SSL_WANT_WRITE) {
                console.log("connect: mbedtls_ssl_handshake: {0}", { hs });
                return null;
            }
        }
        
#if defined(NDEBUG)
        /// verify server ca x509; exit if flags != 0
        int flags = mbedtls_ssl_get_verify_result(&i.ssl);
        if (flags != 0) {
            console.log("connect: trust check failure, disconnecting");
            return null;
        } else
            console.log("connect: trusted, connected");
#else
        console.log("connect: trust check bypass (debug-build)");
#endif
    } else {
        std::cerr << "unsupported proto: " << p.cstr() << "\n";
        assert(false);
    }
    s.uri = a;
    s.connected = true;
    return s;
}

/// listen on https using mbedtls
async Socket::listen(URI url, std::function<void(Socket)> fn)
{
    return async(1, [&, url=url, fn=fn](auto process, int t_index) -> var {
        URI         uri = url.methodize(URI::Method::Get);
        auto  sc_listen = Socket(Server); /// remove the interfacing in this case
        auto         &i = *(sc_listen.intern);
        
        /// state the truth
        assert(uri.proto() == "https");

        /// start listening on adapter address over port (443 = https default but it can be any)
        if (mbedtls_net_bind(&i.ctx, uri.host().cstr(), std::to_string(uri.port()).c_str(), MBEDTLS_NET_PROTO_TCP) != 0)
            return false;
        
        /// boilerplate thing for initialization of a conf struct.  i like its boilers.
        if (mbedtls_ssl_config_defaults(&i.conf, MBEDTLS_SSL_IS_SERVER,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
            return false;
        
        /// more initialization of the random number gen.
        mbedtls_ssl_conf_rng(&i.conf, mbedtls_ctr_drbg_random, &i.ctr_drbg);
        
        /// direct logging to standard out, worf.  aye sir.
        mbedtls_ssl_conf_dbg(&i.conf, logging, stdout);
        
        /// bind certificate authority chain (i think)
        mbedtls_ssl_conf_ca_chain(&i.conf, &i.ca_cert, NULL);
        
        /// bind who we are.
        if (mbedtls_ssl_conf_own_cert(&i.conf, &i.ca_cert, &i.pkey) != 0)
            return false;
        
        for (;;) {
            char    cip[64] = { 0 };
            size_t  cip_len = 0;
            
            /// create client socket first (not used to this!)
            auto  sc_client = Socket(Server);
            auto        &ci = *(sc_client.intern);
            
            /// setup ssl with the configuration of the accepting socket
            if (mbedtls_ssl_setup(&ci.ssl, &i.conf) != 0)
                return false;
            
            console.log("server: accepting new connection...\n");
            
            /// the following should never block the acceptance thread for any undesirable time
            /// accept connection
            if (mbedtls_net_accept(&i.ctx, &ci.ctx, cip, sizeof(cip), &cip_len) != 0)
                return false;
            
            /// tell it where the tubing is
            mbedtls_ssl_set_bio(&ci.ssl, &ci.ctx, mbedtls_net_send, mbedtls_net_recv, NULL);
            
            /// handshake through said tubes
            for (;;) {
                int hs = mbedtls_ssl_handshake(&ci.ssl);
                ///
                if (hs == 0)
                    break;
                ///
                if (hs != MBEDTLS_ERR_SSL_WANT_READ && hs != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    console.log("server: abort during handshake");
                    return false;
                }
            }
            
            /// spawn thread for the given callback
            async(1, [&, sc_client=sc_client, uri=uri, fn=fn](auto process, int t_index) -> var {
                fn(sc_client);
                sc_client.close();
                return true;
            });
        }
        return true;
    });
}
