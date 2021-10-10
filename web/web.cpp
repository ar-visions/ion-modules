#include <web/web.hpp>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sstream>
#include <string.h>

#include <mbedtls/net_sockets.h>
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>


map<str, URI::Method> URI::mmap = {
    { "GET",    URI::Get    },
    { "PUT",    URI::Put    },
    { "POST",   URI::Post   },
    { "DELETE", URI::Delete }
};

URI::Method URI::parse_method(str s) {
    str u = s.to_upper();
    return (!mmap.count(u)) ? Undefined : mmap[u];
};

str URI::encode(str s) {
    static vec<char> chars;
    if (!chars) {
        std::string data = "-._~:/?#[]@!$&'()*+;%=";
        chars = vec<char>(data.data(), data.size());
    }
    // A-Z, a-z, 0-9, -, ., _, ~, :, /, ?, #, [, ], @, !, $, &, ', (, ), *, +, ,, ;, %, and =
    size_t  len = s.length();
    vec<char> v = vec<char>(len * 4 + 1);
    
    for (int i = 0; i < len; i++) {
        const char c = s[i];
        bool a = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
        if (!a)
             a = chars.index_of(c) != 0;
        if (!a) {
            v += '%';
            std::stringstream st;
            if (c >= 16)
                st <<        std::hex << int(c);
            else
                st << "0" << std::hex << int(c);
            std::string n(st.str());
            v += n[0];
            v += n[1];
        } else
            v += c;
        
        if (c == '%')
            v += '%';
    }
    
    return str(v.data(), v.size());
}


str URI::parse_encoded(str e) {
    size_t sz = e.length();
    auto    v = vec<char>(sz * 4 + 1);
    size_t  i = 0;
    auto is_hex = [](const char c) {
        return ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F'));
    };
    while (i < sz) {
        const char c0 = e[i];
        if (c0 == '%') {
            if (i >= sz + 1)
                break;
            const char c1 = e[i + 1];
            if (c1 == '%') {
                v += '%';
            } else {
                if (i >= sz + 2)
                    break;
                const char c2 = e[i + 2];
                if (!is_hex(c1) || !is_hex(c2))
                    v += ' ';
                else {
                    char b[3] = { c1, c2, 0 };
                    std::stringstream ss;
                    ss << std::hex << b;
                    unsigned char vv;
                    ss >> vv;
                    v  += vv; // todo: add some hex parsing utility; should work to return strings or data
                }
            }
        } else if (c0 == '+') {
            v += ' ';
        } else
            v += c0;
        i++;
    }
    return str(v.data(), v.size());
}

Args Web::read_headers(Socket sc) {
    Args headers;
    int line = 0;
    for (;;) {
        vec<char> rbytes = sc.read_until({"\r\n"}, 8192);
        size_t sz = rbytes.size();
        if (sz == 0)
            return false;
        if (sz == 2)
            break;
        if (line++ == 0) {
            headers["Status"]  = str(rbytes);
        } else {
            for (size_t i = 0; i < sz; i++) {
                if (rbytes[i] == ':') {
                    str k      = str(&rbytes[0], i);
                    str v      = str(&rbytes[i + 2], sz - k.length() - 2 - 2);
                    headers[k] = v;
                    break;
                }
            }
        }
    }
    return headers;
}

bool Web::write_headers(Socket sc, Args &headers) {
    for (auto &[k,v]: headers)
        if (!sc.write("{0}: {1}", {k, v}))
             return false;
    if (!sc.write("\r\n"))
         return false;
    return true;
}

Data Web::content_data(Socket sc, Data &c, Args& headers) {
    if (c.t == Data::Map || c.t == Data::Array)
        headers["Content-Type"] = "application/json";
    str s = std::string(c);
    return Data(s);
}

Data Web::read_content(Socket sc, Args& headers) {
    const char      *te = "Transfer-Encoding";
    const char      *cl = "Content-Length";
  //const char      *ce = "Content-Encoding";
    int            clen = headers.count(cl) ? str(headers[cl]).integer() : -1;
    bool        chunked = headers.count(te) && headers[te] == Data("chunked");
    ssize_t        tlen = clen;
    ssize_t        rlen = 0;
    const ssize_t r_max = 1024;
    bool          error = false;
    int            iter = 0;
    vec<uint8_t> v_data;
    assert(!(clen >= 0 && chunked));
    
    if (!(!chunked && clen == 0)) {
        do {
            if (chunked) {
                if (iter++ > 0) {
                    char crlf[2];
                    if (!sc.read(crlf, 2) || memcmp(crlf, "\r\n", 2) != 0) {
                        error = true;
                        break;
                    }
                }
                vec<char> rbytes = sc.read_until({"\r\n"}, 64);
                if (!rbytes) {
                    error = true;
                    break;
                }
                std::stringstream ss;
                ss << std::hex << rbytes.data();
                ss >> tlen;
                if (tlen == 0)
                    break;
            }
            for (ssize_t rcv = 0; tlen == -1 || rcv < tlen; rcv += rlen) {
                ssize_t rx = tlen > 0 ? std::min(tlen - rcv, r_max) : r_max;
                uint8_t buf[r_max];
                rlen = sc.read_raw((const char *)buf, rx);
                if (rlen > 0) {
                    v_data.expand(rlen, 0);
                    memcpy(&v_data[v_data.size() - rlen], buf, rlen);
                } else if (rlen < 0) {
                    error = tlen < 0;
                    break;
                }
            }
        } while (!error && chunked && tlen != 0);
    }
    return error ? Data(null) : Data(v_data);
}

#if !defined(_WIN32)
#define closesocket close
typedef int SOCKET;
#endif

struct SocketInternal {
    bool                     secure;
    SOCKET                   fd_insecure;
    mbedtls_x509_crt         ca_cert;
    mbedtls_net_context      ctx;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_pk_context       pkey; // for server only
};

vec<std::filesystem::path> files(std::filesystem::path p, vec<str> exts = {}) {
    vec<std::filesystem::path> v;
    if (std::filesystem::is_directory(p)) {
        for (const auto &f: std::filesystem::directory_iterator(p))
            if (f.is_regular_file() && (!exts || exts.index_of(f.path().string()) >= 0))
                v += f.path();
    } else if (std::filesystem::exists(p))
        v += p;
    return v;
}

Socket::Socket(nullptr_t n) { }

Socket::Socket(bool secure, bool listen) {
    intern = std::shared_ptr<SocketInternal>(new SocketInternal);
    auto &i = *(intern.get());
    memset(intern.get(), 0, sizeof(SocketInternal));
    intern->secure = secure;
    if (secure) {
        const char *pers = "ion:web";
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
        std::filesystem::path path = listen ? "trust/server.cer" : "trust";
        auto vf = files(path, {".cer"});
        for (auto &p: vf) {
            int r = mbedtls_x509_crt_parse_file(&i.ca_cert, p.string().c_str());
            if (r != 0) {
                std::cerr << "mbedtls_x509_crt_parse_file failure: " << p << std::endl;
                exit(1);
            }
        }
        
        std::filesystem::path sv_path = "server.key";
        if (std::filesystem::exists(sv_path)) {
            std::string p = sv_path.string();
            int r = mbedtls_pk_parse_keyfile(&i.pkey, p.c_str(),
                        nullptr, mbedtls_ctr_drbg_random, &i.ctr_drbg);
            if (r != 0) {
                std::cerr << "mbedtls_x509_crt_parse_file failure: " << r << std::endl;
                exit(1);
            }
        }
        
        mbedtls_ssl_conf_authmode(&i.conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    }
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

int Socket::read_raw(const char *v, size_t sz) {
    SocketInternal &i = *intern;
    int r = i.secure ? mbedtls_ssl_read(&i.ssl, (unsigned char *)v, sz) :
                                   recv(i.fd_insecure, (void *)v, sz, 0);
    return r;
}

bool Socket::read(const char *v, size_t sz) {
    SocketInternal &i = *intern;
    int            st = 0;
    ///
    for (int len = sz; len > 0;) {
        int r = i.secure ? mbedtls_ssl_read(&i.ssl, (unsigned char *)&v[st], len) :
                                       recv(i.fd_insecure, (void *)&v[st], len, 0);
        if (r <= 0) // -28928
            return false;
        len -= r;
        st  += r;
    }
    return true;
}

vec<char> Socket::read_until(str s, int max_len) {
    auto rbytes = vec<char>(max_len);
    size_t slen = s.length();
    ///
    for (;;) {
        rbytes   += '\0';
        size_t sz = rbytes.size();
        if (!read(&rbytes[sz - 1], 1))
            return vec<char> { };
        if (sz >= slen && memcmp(&rbytes[sz - slen], s.cstr(), slen) == 0)
            break;
        if (sz == max_len)
            return vec<char> { };
    }
    return rbytes;
}

bool Socket::write(const char *v, size_t sz, int flags) {
    SocketInternal &i = *intern;
    size_t tlen = sz;
    size_t  len = tlen;
    size_t ibuf = 0;
    ///
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

bool Socket::write(vec<char> &v) {
    return write(v.data(), v.size(), 0);
}

bool Socket::write(str s, std::vector<Data> a) {
    str f = str::format(s, a);
    return write(f.cstr(), f.length(), 0);
}

bool Socket::write(Data &v) {
    if (v == Data::Str)
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

void Socket::logging(void *ctx, int level, const char *file, int line, const char *str) {
    fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
}

Socket Socket::connect(str uri) {
    URI    a = URI::parse(str("GET ") + uri);
    str   qy = a.query;
    Socket s = null;
    str    p = a.proto;
    str    h = a.host;
    int port = a.port;
    ///
    if (p == "https") {
        s = Socket(true, false);
        SocketInternal &i = *(s.intern);
        
        if (mbedtls_net_connect(
                &i.ctx, h.cstr(), std::to_string(port).c_str(), MBEDTLS_NET_PROTO_TCP) != 0)
            return null;
        
        if (mbedtls_ssl_config_defaults(
                &i.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
            return null;
        
        /// set verification auth mode
        mbedtls_ssl_conf_ca_chain(&i.conf, &i.ca_cert, NULL);
        mbedtls_ssl_conf_rng(&i.conf, mbedtls_ctr_drbg_random, &i.ctr_drbg);
        mbedtls_ssl_conf_dbg(&i.conf, logging, stdout);

        if (mbedtls_ssl_setup(&i.ssl, &i.conf) != 0)
            return null;

        if (mbedtls_ssl_set_hostname(&i.ssl, h.cstr()) != 0)
            return null;

        mbedtls_ssl_set_bio(&i.ssl, &i.ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

        console.log("handshaking...");
        
        /// handshake
        for (;;) {
            int hs = mbedtls_ssl_handshake(&i.ssl);
            if (hs == 0)
                break;
            if (hs != MBEDTLS_ERR_SSL_WANT_READ && hs != MBEDTLS_ERR_SSL_WANT_WRITE)
                return null;
        }
        
        console.log("handshook.");
        
        /// verify server ca x509; exit if flags != 0
        int flags = mbedtls_ssl_get_verify_result(&i.ssl);
#if !defined(MBEDTLS_X509_REMOVE_INFO)
        if (flags != 0) {
            char vrfy_buf[512];
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
        }
#endif
    } else if (p == "http") {
        struct sockaddr_in server_addr;
        
        /// lookup host by name
        struct hostent *server_host = gethostbyname(h.cstr());
        if (!server_host)
            return null;
        
        /// create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_fd < 0)
            return null;
        
        s = Socket(false, false);
        SocketInternal &i = *(s.intern);
        i.fd_insecure = server_fd;
        
        /// connect to server
        memcpy((void *)&server_addr.sin_addr,
               (void *) server_host->h_addr,
                        server_host->h_length);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(uint16_t(port));
        int cret               = ::connect(i.fd_insecure,
                                          (struct sockaddr *)&server_addr,
                                           sizeof(server_addr));
        if (cret < 0)
            return null;
    } else {
        std::cerr << "unsupported proto: " << p.cstr() << "\n";
        assert(false);
    }
    s.uri = a;
    s.connected = true;
    return s;
}

Future Web::request(str s_uri, Args args) {
    return Async(1, [s_uri=s_uri, args=args](auto p, int i) -> Data {
        Args            &a = *(Args *)&args;
        str       s_method = a.count("method")    ? str(a["method"]) : str{"GET"};
        URI::Method method = s_method == "GET"    ? URI::Get    :
                             s_method == "POST"   ? URI::Post   :
                             s_method == "PUT"    ? URI::Put    :
                             s_method == "DELETE" ? URI::Delete : URI::Undefined;
        Args*      headers = a.count("headers")   ? (Args *) a["headers"] : nullptr;
        Data*      content = a.count("content")   ? (Data *)&a["content"] : nullptr;
        Message     result = {URI::Response};
        
        assert(method != URI::Undefined);
        assert(method != URI::Get || content == null);
        
        /// connect to server (secure or insecure depending on uri)
        console.log("connecting to {0}", {s_uri});
        Socket sc = Socket::connect(s_uri);
        if (!sc)
            return null;
        
        /// write query
        str s_query = sc.uri.query;
        sc.write("{0} {1} HTTP/1.1\r\n", {s_method, s_query});
        
        /// write headers
        if (!headers || headers->count("User-Agent") == 0)
            sc.write("User-Agent: ion\r\n");
        if (!headers || headers->count("Host") == 0)
            sc.write("Host: {0}\r\n", {sc.uri.host});
        if (!headers || headers->count("Accept-Language") == 0)
            sc.write("Accept-Language: en-us\r\n");
        if (!headers || headers->count("Accept-Encoding") == 0)
            sc.write("Accept-Encoding: gzip, deflate, br\r\n");
        if (!headers || headers->count("Accept") == 0)
            sc.write("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
        
        assert(method == URI::Post || (!headers || headers->count("Content-Length") == 0));
        size_t plen = content ? content->size() : 0;
        sc.write("Content-Length: {0}\r\n", {int(plen)});
        if (headers)
            for (auto &[k,v]: *headers)
                sc.write("{0}: {1}\r\n", {k, v});
        
        /// end message
        sc.write("\r\n");
        
        /// write content data
        if (plen > 0) {
            // todo: support serializing this content into json object
            const char *send_data = content->data<const char>();
            size_t      send_len  = content->size();
            sc.write(send_data, send_len, 0);
        }
        
        /// read headers
        auto  &r = result.headers = Web::read_headers(sc);
        str   ct = r.count("Content-Type") ? str(r["Content-Type"]) : str("");
        Data rcv = read_content(sc, result.headers);
        
        /// read content
        if (ct == "application/json") {
            const char *c = rcv.data<const char>();
            size_t     sz = rcv.size();
            str        js = str(c, sz);
            result.content = Data(Data::Map);
            result.content.read_json(js); // eh.
        } else
            result.content = rcv;
        
        sc.close();
        return result;
    });
}

Future Web::json(str resource, Args args, Args headers) {
    std::function<void(Data &)> s, f;
    Completer c = { s, f };
    Web::request(resource, headers).then([s, f](Data &d) {
        auto &headers = d["headers"];
        str ct = headers["Content-Type"];
        (ct == "application/json") ? s(d) : f(d);
    }).except([&](Data &d) {
        f(d);
    });
    return Future(c);
}

Async Socket::listen(str uri, std::function<void(Socket)> fn) {
    return Async(1, [&, uri=uri, fn=fn](auto process, int t_index) -> Data {
        auto          a = URI::parse(str("GET ") + uri);
        auto  sc_listen = Socket(true, true);
        auto         &i = *(sc_listen.intern);
        str           p = a.proto;
        str           h = a.host;
        int        port = a.port;
        
        if (p == "https") {
            /// to me this is probably the least efficient way of connecting and storing the ssl state
            /// but if it goes wrong, its definitely less cross contaminable
            if (mbedtls_net_bind(&i.ctx, h.cstr(), std::to_string(port).c_str(), MBEDTLS_NET_PROTO_TCP) != 0)
                return false;
            if (mbedtls_ssl_config_defaults(&i.conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
                return false;
            
            mbedtls_ssl_conf_rng(&i.conf, mbedtls_ctr_drbg_random, &i.ctr_drbg);
            mbedtls_ssl_conf_dbg(&i.conf, logging, stdout);
            mbedtls_ssl_conf_ca_chain(&i.conf, &i.ca_cert, NULL);
            
            /// load in cert we use with private key.
            if (mbedtls_ssl_conf_own_cert(&i.conf, &i.ca_cert, &i.pkey) != 0)
                return false;
        }
        
        for (;;)
            if (p == "https") {
                char    cip[64] = { 0 };
                size_t  cip_len = 0;
                
                /// create client socket first (not used to this!)
                auto  sc_client = Socket(true, true);
                auto        &ci = *(sc_client.intern);
                
                /// setup ssl with the configuration of the accepting socket
                if (mbedtls_ssl_setup(&ci.ssl, &i.conf) != 0)
                    return false;
                
                /// accept connection
                if (mbedtls_net_accept(&i.ctx, &ci.ctx, cip, sizeof(cip), &cip_len) != 0)
                    return false;
                
                /// tell it where the tubing is
                mbedtls_ssl_set_bio(&ci.ssl, &ci.ctx, mbedtls_net_send, mbedtls_net_recv, NULL);
                
                /// handshake through said tubes
                for (;;) {
                    int hs = mbedtls_ssl_handshake(&ci.ssl);
                    if (hs == 0)
                        break;
                    if (hs != MBEDTLS_ERR_SSL_WANT_READ && hs != MBEDTLS_ERR_SSL_WANT_WRITE)
                        return false;
                }

                /// spawn thread for the given callback
                Async(1, [&, sc_client=sc_client, uri=uri, fn=fn](auto process, int t_index) -> Data {
                    fn(sc_client);
                    sc_client.close();
                    return true;
                });
            }
        return true;
    });
}

Async Web::server(str l_uri, std::function<Message(Message &)> fn) {
    return Socket::listen(l_uri, [&, l_uri=l_uri, fn=fn](Socket sc) {
        for (bool close = false; !close;) {
            close       = true;
            vec<char> m = sc.read_until("\r\n", 4092);
            str   param;
            if (!m)
                return false;
            Message msg;
            str s_uri = str(m.data(), m.size() - 2);
              msg.uri = URI::parse(s_uri);

            if (msg && msg.uri.resource) {
                msg.headers = Web::read_headers(sc);
                msg.content = Web::read_content(sc, msg.headers);
                close       = msg.headers["Connection"] == Data("close");
                Message res = fn(msg);
                if (res && res.content) {
                    Data    c = Web::content_data(sc, res.content, res.headers);
                    size_t sz = c.size();
                    res.headers["Content-Length"] = std::to_string(sz);
                    res.headers["Connection"]     = "keep-alive";
                    Web::write_headers(sc, res.headers);
                    sc.write(c);
                } else {
                    res.headers["Content-Length"] = "0";
                    res.headers["Connection"]     = "keep-alive";
                    Web::write_headers(sc, res.headers);
                }
            }
        }
        
        // needs loop.
        return true;
    });
}
