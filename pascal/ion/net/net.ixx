module;

#include <core/core.hpp>
///
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>
///
#ifdef _WIN32
#   include <Winsock2.h>
#else
#   include <arpa/inet.h>
#endif
///
#include <errno.h>

#undef min
#undef max

export module net;

import core;
import async;

template <typename T>
bool be_a_penis(T &arg) {
    constexpr bool has_etype2 = requires(const T& t) { (typename T::etype)t; };
    return has_etype2;
}

export {

enums(method, undefined, 
   "undefined, response, get, post, put, delete",
    undefined, response, get, post, put, del);

struct uri:mx {
protected:
    struct components {
        method  mtype; /// good to know where the uri comes from, in what context (what method, this is part of uri because method is Something that makes it uniquely bindable; losing that makes you store it somewhere else and who wants that)
        str     proto;
        str     host;
        int     port;
        str     query;
        str     resource;
        map<mx> args;
        str     version;
    };
    ///
    components& a;

public:
    /// singular container type used for 'enumeration' and containment of value: method
    /// enum = method::mtype
    /// default = undefined, then the arg list
    ///
    method & mtype() { return a.mtype;    }
    str &     host() { return a.host;     }
    str &    proto() { return a.proto;    }
    int &     port() { return a.port;     }
    str &   string() { return a.query;    }
    str & resource() { return a.resource; }
    map<mx> & args() { return a.args;     }
    str &  version() { return a.version;  }

    ///
    static str encode_fields(map<mx> fields) {
        method m;
        be_a_penis(m);
        str post(size_t(1024)); /// would be interesting to keep track of the data size in a map, thats sort of a busy-work routine for N users and the container struct wouldnt be conflicted with the data
        if (fields) {
            for (auto& [val, key] : fields) {
                str s_key = key.grab();
                str s_val = val.grab();
                if (post)
                    post += "&";
                post += str::format("{0}={1}", {uri::encode(s_key), "=", uri::encode(s_val)});
            }
        }
        return post;
    }

    ctr(uri, mx, components, a);

    static memory *convert(memory *mem) {
        return parse(str(mem));
    }

    uri(str                s) : uri(parse(s))   { }
    uri(symbol           sym) : uri(parse(sym)) { } 
    uri(uri &r, method mtype) : uri(r.copy()) { a.mtype = mtype; }

    ///
    operator str () {
        struct def {
            symbol proto;
            int        port;
        };
        
        array<def>   defs = {{ "https", 443 }, { "http", 80 }};
        bool       is_def = defs.select_first<bool>([&](def& d) -> bool {
            return a.port == d.port && a.proto == d.proto;
        });

        str      s_port   = !is_def ? str::format(":{0}", { a.port })                : "";
        str      s_fields =  a.args ? str::format("?{0}", { encode_fields(a.args) }) : "";

        if (a.args) /// if there is query str, append ?{encoded}
            s_fields      = str::format("?{0}", { encode_fields(a.args) });
        
        /// return string uri
        return str::format("{0}://{1}{2}{3}{4}", { a.proto, a.host, s_port, a.query, s_fields });
    }

    uri methodize(method mtype) const {
        a.mtype = mtype;
        return *this;
    }

    bool operator==(method::etype m) { return a.mtype.value == m; }
    bool operator!=(method::etype m) { return a.mtype.value != m; }
    operator           bool() { return a.mtype.value != method::undefined; }

    /// can contain: GET/PUT/POST/DELETE uri [PROTO] (defaults to GET)
    static memory *parse(str raw, uri* ctx = null) {
        array<str> sp   = raw.split(" ");
        uri    result;
        components& ra  = result.a;
        bool   has_meth = sp.len() > 1;
        str lc = sp.len() > 0 ? sp[0].lcase() : str();
        method m        = method(has_meth ? lc.cs() : cstr("get"));
        str    u        =     sp[has_meth ? 1 : 0];
        ra.mtype        = m;

        /// find a distraught face
        int iproto      = u.index_of("://");
        if (iproto >= 0) {
            str       p = u.mid(0, iproto);
            u           = u.mid(iproto + 3);
            int ihost   = u.index_of("/");
            ra.proto    = p;
            ra.query    = ihost >= 0 ? u.mid(ihost) : str("/");
            str       h = ihost >= 0 ? u.mid(0, ihost) : u;
            int      ih = h.index_of(":");
            u           = ra.query;
            if (ih > 0) {
                ra.host = h.mid(0, ih);
                ra.port = int(h.mid(ih + 1).integer_value());
            } else {
                printf("h = %s\n", h.cs());
                ra.host = h;
                ra.port = p == "https" ? 443 : 80;
            }
        } else {
            /// return default
            ra.proto    = ctx ? ctx->a.proto : "";
            ra.host     = ctx ? ctx->a.host  : "";
            ra.port     = ctx ? ctx->a.port  : 0;
            ra.query    = u;
        }
        /// parse resource and query
        auto iq = u.index_of("?");
        if (iq > 0) {
            ra.resource  = decode(u.mid(0, iq));
            str        q = decode(u.mid(iq + 1));
            array<str> a = q.split(",");
            ra.args      = map<mx>();
            for (str &kv:  a) {
                array<str> a = kv.split("=");
                mx    &k = a[0];
                mx    &v = a.len() > 1 ? a[1] : k;
                ra.args[k] = v;
            }
        } else
            ra.resource = decode(u);
        ///
        if (sp.len() >= 3)
            ra.version  = sp[2];
        ///
        return result.grab();
    }

    ///
    static str encode(str s) {
        static const char *s_chars = " -._~:/?#[]@!$&'()*+;%=";
        static array<char>   chars { (char*)s_chars, strlen(s_chars) };

        /// A-Z, a-z, 0-9, -, ., _, ~, :, /, ?, #, [, ], @, !, $, &, ', (, ), *, +, ,, ;, %, and =
        size_t  len = s.len();
        array<char> v = array<char>(len * 4 + 1); 
        for (size_t i = 0; i < len; i++) {
            cchar_t c = s[i];
            bool    a = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
            if (!a)
                 a    = chars.index_of(c) != 0;
            if (!a) {
                v    += '%';
                std::stringstream st;
                if (c >= 16)
                    st << std::hex << int(c);
                else
                    st << "0" << std::hex << int(c);
                str n(st.str());
                v += n[0];
                v += n[1];
            } else
                v += c;
            if (c == '%')
                v += '%';
        }
        return str(v.data(), int(v.len()));
    }

    static str decode(str e) {
        size_t     sz = e.len();
        array<char> v = array<char>(size_t(sz * 4 + 1));
        size_t      i = 0;

        while (i < sz) {
            cchar_t c0 = e[i];
            if (c0 == '%') {
                if (i >= sz + 1)
                    break;
                cchar_t c1 = e[i + 1];
                if (c1 == '%')
                    v += '%';
                else {
                    if (i >= sz + 2)
                        break;
                    cchar_t c2 = e[i + 2];
                    v += str::char_from_nibs(c1, c2);
                }
            } else
                v += (c0 == '+') ? ' ' : c0;
            i++;
        }
        return str(v.data(), int(v.len()));
    }
};

struct sock:mx {
    ///
    struct intern_t {
        uri                      query;
        i64                      timeout_ms;
        bool                     connected;

        int                      listen_fd, sock_fd;
        WOLFSSL_CTX*             ctx;
        WOLFSSL*                 ssl;
        WOLFSSL_METHOD*          method;
        sockaddr_in              bound;
    } &i;

    ctr(sock, mx, intern_t, i);

    ///
    uri       query() const { return  i.query;      }
    bool timeout_ms() const { return  i.timeout_ms; }
    bool  connected() const { return  i.connected;  }
    bool  operator!() const { return !i.connected;  };
    operator   bool() const { return  i.connected;  };

    ///
    enum role {
        client,
        server
    };

    ///
    void set_timeout(i64 t) {
        i.timeout_ms = t;
    }

    sock(role r, uri bind) : sock() {
        /// if its release it must be secure
        symbol pers = "mx:net";
        static bool wolf;
        if (!wolf) {
            static WSADATA wsa_data;
            static int wsa = WSAStartup(MAKEWORD(2,2), &wsa_data);
            if (wsa != 0) {
                printf("(sock) WSAStartup failed: %d\n", wsa);
                return;
            }
            wolf = true; /// wolves should always be true
            wolfSSL_Init(); 
        }

        /// set method based on role
        const bool is_server = (r == role::server);
        i.method = is_server ? wolfTLSv1_2_server_method() : wolfTLSv1_2_client_method();

        /// create ctx based on method
        if ((i.ctx = wolfSSL_CTX_new(i.method)) == NULL)
            err_sys("wolfSSL_CTX_new error");
        
        /// load cert and key based on role
        str rtype = is_server ? "server" : "client";
        str cert  = str::format("certs/{0}-cert.pem", { rtype });
        str key   = str::format("certs/{0}-key.pem",  { rtype });

        /// load cert (public)
        if (wolfSSL_CTX_use_certificate_file(i.ctx, cert.cs(), SSL_FILETYPE_PEM) != SSL_SUCCESS)
            console.fault("error loading {0}", { cert });
        /// load key (private)
        if (wolfSSL_CTX_use_PrivateKey_file (i.ctx, key.cs(),  SSL_FILETYPE_PEM) != SSL_SUCCESS)
            console.fault("error loading {0}", { key });

        size_t addr_sz = sizeof(sockaddr_in);
        uint16_t  port = (uint16_t)bind.port();
        str   &ip_addr = bind.host(); /// todo: convert if not address
        i.query        = bind;
        i.sock_fd      = socket(AF_INET, SOCK_STREAM, 0);
        i.bound        = sockaddr_in { AF_INET, htons(port) };
        inet_pton(AF_INET, ip_addr.cs(), &i.bound.sin_addr);

        /// if this is a server, block until a new connection arrives
        if (is_server) {
            printf("listening on %d\n", int(port));
          ::bind   (i.sock_fd, (sockaddr *)&i.bound, sizeof(sockaddr_in));
          ::listen (i.sock_fd, 0);
        } else {
            str qy = i.query.string();
            printf("connect to %s\n", qy.cs());
          ::connect(i.sock_fd, (sockaddr *)&i.bound, sizeof(sockaddr_in));
            establish();
        }
    }

    sock accept() {
        sock  sc = i;
        int   addr_sz = sizeof(sockaddr_in);
        sc.i.bound.sin_addr.S_un.S_addr = INADDR_ANY;
        sc.i.sock_fd = ::accept(i.sock_fd, (sockaddr*)&sc.i.bound, &addr_sz);
        printf("accepted: %i:%i\n", sc.i.sock_fd, WSAGetLastError());
        return sc.establish();
    }

    sock &establish() {
        assert(!i.ssl);
        if (i.sock_fd > 0) {
            /// create wolf-ssl object
            if ((i.ssl = wolfSSL_new(i.ctx)) == NULL)
                err_sys("wolfSSL_new error");

            wolfSSL_set_fd(i.ssl, i.sock_fd);
            i.connected = true;
        } else {
            printf("could not establish ssl\n");
        }
        return *this;
    }

    int read(u8 *v, size_t sz) {
        int    len = int(sz);
        int    rcv = wolfSSL_read(i.ssl, v, len);
        return rcv;
    }

    bool read_sz(u8 *v, size_t sz) {
        int st = 0;
        ///
        for (int len = int(sz); len > 0;) {
            int rcv = wolfSSL_read(i.ssl, &v[st], len);
            if (rcv <= 0)
                return false;
            len -= rcv;
            st  += rcv;
        }
        return true;
    }

    array<char> read_until(str s, int max_len) {
        auto rbytes = array<char>(size_t(max_len));
        size_t slen = s.len();
        ///
        for (;;) {
            rbytes   += '\0';
            size_t sz = rbytes.len();
            if (!read((u8*)&rbytes[sz - 1], 1))
                return array<char> { };
            if (sz >= slen and memcmp(&rbytes[sz - slen], s.cs(), slen) == 0)
                break;
            if (sz == max_len)
                return array<char> { };
        }
        return rbytes;
    }

    bool write(u8 *v, size_t sz) {
        assert(sz > 0);
        int      len = int(sz);
        int     wlen = wolfSSL_write(i.ssl, v, len);
        return (wlen == sz);
    }

    bool write(array<char> v) {
        return write((u8*)v.data(), v.len());
    }

    bool write(str s, array<mx> a) {
        str f = str::format(s.cs(), a);
        return write((u8*)f.cs(), f.len());
    }

    bool write(str s) {
        return write((u8*)s.cs(), s.len());
    }

    /// for already string-like memory; this could do something with the type on mx
    bool write(mx &v) {
        return write((u8*)v.mem->origin, v.count() * v.type_size());
    }

    void close() const {
#if _WIN32
        closesocket((SOCKET)i.sock_fd);
#else
        close(i.sock_fd);
#endif
        wolfSSL_CTX_free(i.ctx);
      //wolfSSL_Cleanup();
    }

    /// ---------------------------------
    static void logging(void *ctx, int level, symbol file, int line, symbol str) {
        fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
    }
    
    /// ---------------------------------
    static sock connect(uri u) {
        uri a = u.methodize(method::get);
        return sock(role::client, a);
    }

    /// listen on https using mbedtls
    /// ---------------------------------
    static async listen(uri url, lambda<bool(sock&)> fn) {
        return async(1, [&, url, fn](runtime &rt, int i) -> mx {
            uri bind = url.methodize(method::get);

            /// proceed if https; that is our protocol and we know nothing else
            assert(bind.proto() == "https");

            int      port = bind.port();
            str host_name = bind.host();
            console.log("(net) listen on: {0}:{1}", { host_name, port });
            sock server { role::server, bind };

            /// loop around grabbing anyone who comes-a-waltzing
            while (sock client = server.accept()) {
                /// spawn thread for the given callback -- this lets us accept again right away, on this thread
                async {1, [client=client, fn=fn](runtime &rt, int i) -> mx {
                    if (fn((sock&)client)) client.close();
                    return true;
                }};
            }
            return true;
        });
    }
};

struct message:mx {

    struct members {
        uri     query;
        mx      code = int(0);
        map<mx> headers;
        mx      content; /// best to store as mx, so we can convert directly in lambda arg, functionally its nice to have delim access in var.
    } &m;

    method method_type() {
        return m.query.mtype();
    }

    ctr(message, mx, members, m);

    message(int server_code) : message() {
        m.code = mx(server_code);
    }

    message(symbol text) : message() {
        m.content = text;
        m.code    = 200;
    }

    message(str text) : message() {
        m.content = text;
        m.code    = 200;
    }

    message(path p, mx modified_since = {}) : message() {
        m.content = p.read<str>();
        m.headers["Content-Type"] = p.mime_type();
        m.code    = 200;
    }

    message(mx content, map<mx> headers = {}, uri query = {}) : message() {
        m.query   = query;
        m.headers = headers;
        m.content = content; /// assuming the content isnt some error thing
        m.code    = 200;
    }

    message(uri url, map<mx> headers = {}) : message() {
        m.query   = url;
        m.headers = headers;
    }

    /// important to note we do not keep holding onto this socket
    message(sock sc) : message() {
        read_headers(sc);
        read_content(sc);
        m.code = m.headers["Status"];
    }

    uri query() { return m.query; }

    bool read_headers(sock sc) {
        int line = 0;
        ///
        for (;;) {
            array<char> rbytes = sc.read_until({ "\r\n" }, 8192);
            size_t sz = rbytes.len();
            if (sz == 0)
                return false;
            ///
            if (sz == 2)
                break;
            ///
            if (line++ == 0) {
                str hello = str(rbytes.data(), int(sz - 2));
                i32  code = 0;
                auto   sp = hello.split(" ");
                if (hello.len() >= 12) {
                    if (sp.len() >= 2)
                        code = i32(sp[1].integer_value());
                }
                m.headers["Status"] = str::from_integer(code);
                ///
            } else {
                for (size_t i = 0; i < sz; i++) {
                    if (rbytes[i] == ':') {
                        str k = str(&rbytes[0], int(i));
                        str v = str(&rbytes[i + 2], int(sz) - int(k.len()) - 2 - 2);
                        m.headers[k] = v;
                        break;
                    }
                }
            }
        }
        return true;
    }

    ///
    bool read_content(sock sc) {
        str              te = "Transfer-Encoding";
        str              cl = "Content-Length";
        //cchar_t         *ce = "Content-Encoding";
        int            clen = int(m.headers.count(cl) ? str(m.headers[cl].grab()).integer_value() : -1);
        bool        chunked =     m.headers.count(te) &&    m.headers[te] == var("chunked");
        num     content_len = clen;
        num            rlen = 0;
        const num     r_max = 1024;
        bool          error = false;
        int            iter = 0;
        array<uint8_t> v_data;
        ///
        assert(!(clen >= 0 and chunked));
        ///
        if (!(!chunked and clen == 0)) {
            do {
                if (chunked) {
                    if (iter++ > 0) {
                        u8 crlf[2];
                        if (!sc.read_sz(crlf, 2) || memcmp(crlf, "\r\n", 2) != 0) {
                            error = true;
                            break;
                        }
                    }
                    array<char> rbytes = sc.read_until({ "\r\n" }, 64);
                    if (!rbytes) {
                        error = true;
                        break;
                    }
                    std::stringstream ss;
                    ss << std::hex << rbytes.data();
                    ss >> content_len;
                    if (content_len == 0) /// this will effectively drop out of the while loop
                        break;
                }
                num   rx = math::min(1, 1);
                bool sff = content_len == -1;
                for (num rcv = 0; sff || rcv < content_len; rcv += rlen) {
                    num   rx = math::min(1, 1);
                    uint8_t  buf[r_max];
                    rlen = sc.read(buf, rx);
                    if (rlen > 0)
                        v_data.push(buf, rlen);
                    else if (rlen < 0) {
                        error = !sff;
                        /// it is an error if we expected a certain length
                        /// if the connection is closed during a read then
                        /// it just means the peer ended the transfer (hopefully at the end)
                        break;
                    } else if (rlen == 0) {
                        error = true;
                        break;
                    }
                }
            } while (!error and chunked and content_len != 0);
        }
        ///
        mx    rcv = error ? mx() : mx(v_data);
        str ctype = m.headers.count("Content-Type") ? str(m.headers["Content-Type"].grab()) : str("");
        ///
        if (ctype.split(";").count("application/json")) {
            /// read content
            size_t sz = rcv.byte_len();
            str    js = str(&rcv.ref<char>(), int(sz));
            var   obj = var::json(js);
            m.content = obj;
        }
        else if (ctype.starts_with("text/")) {
            m.content = str(&rcv.ref<char>(), int(rcv.byte_len()));
        } else {
            /// other non-text types must be supported, not going to just leave them as array for now
            assert(v_data.len() == 0);
            m.content = var(null);
        }
        return !error;
    }

    /// query/request construction
    static message query(uri server, map<mx> headers, mx content) {
        /// default state
        message m;

        /// acquire members, and set
        message::members& q = m;
        q.query   = { server, method::get };
        q.headers = headers;
        q.content = content;

        /// return state
        return m;
    }

    /// response construction, uri is not needed
    static message response(uri query, mx code, mx content, map<mx> headers = null) {
        message rs;
        message::members& r = rs;
        r.query    = { query, method::response };
        r.code     = code;
        r.headers  = headers;
        r.content  = content;
        return rs;
    }

    explicit operator bool() {
        type_t ct = m.code.type();
        assert(ct == typeof(i32) || ct == typeof(str));
        return (m.query.mtype() != method::undefined) &&
                ((ct == typeof(i32) && int(m.code) >= 200 && int(m.code) < 300) ||
                 (ct == typeof(str) && m.code.byte_len()));
    }

    bool operator!() { return !(operator bool()); }
    
    bool printable_value(mx &v) {
        return v.type() != typeof(mx) || v.count() > 0;
    }

    static symbol code_symbol(int code) {
        static map<symbol> symbols = {
            {200, "OK"}, {201, "Created"}, {202, "Accepted"}, {203, "Non-Authoritative Information"},
            {204, "No Content"}, {205, "Reset Content"}, {206, "Partial Content"},
            {300, "Multiple Choices"}, {301, "Moved Permanently"}, {302, "Found"},
            {303, "See Other"}, {304, "Not Modified"}, {307, "Temporary Redirect"},
            {308, "Permanent Redirect"}, {400, "Bad Request"}, {402, "Payment Required"},
            {403, "Forbidden"}, {404, "Not Found"}, {500, "Internal Server Error"},
            {0,   "Unknown"}
        };
        return symbols.count(code) ? symbols[code] : symbols[0];
    }

    ///  code is in headers.
    bool write_headers(sock sc) {
        mx status = "Status";
        int  code = m.headers.count(status) ? int(m.headers[status]) : int(m.code ? m.code : 200);
        ///
        sc.write("HTTP/1.1 {0} {1}\r\n", { code, code_symbol(code) }); /// needs alias for code
        ///
        for (auto &[v, k]: m.headers) { /// mx key, mx value
            /// check if header is valid data; holding ref to mx 
            /// requires one to defer valid population of a 
            /// resulting header injected by query
            if (k == "Status" || !printable_value(v))
                continue;

            console.log("{0}: {1}", { k, v });

            ///
            if (!sc.write("{0}: {1}", { k, v }))
                return false;
            ///
            if (!sc.write("\r\n"))
                return false;
        }
        if (!sc.write("\r\n"))
            return false;
        ///
        return true;
    }

    bool write(sock sc) {
        if (m.content) {
            type_t ct = m.content.type();
            /// map of mx must be json compatible, or be structured in that process
            if (!m.headers.count("Content-Type") && (ct == typeof(map<mx>) || ct == typeof(array<mx>))) 
                m.headers["Content-Type"] = "application/json";
            ///
            m.headers["Connection"] = "keep-alive";

            /// do different things based on type
            if (m.headers.count("Content-Type") && m.headers["Content-Type"] == "application/json") {
                /// json transfer; content can be anything, its just mx..
                /// so we convert to json explicitly. var is just an illusion as you can see friend.
                str json = var(m.content).stringify();
                m.headers["Content-Length"] = str(json.len());
                write_headers(sc);
                return sc.write(json);
                ///
            } else if (ct == typeof(map<mx>)) {
                /// post fields transfer
                str post = uri::encode_fields(map<mx>(m.content));
                m.headers["Content-Length"] = str::from_integer(post.len());
                write_headers(sc);
                return sc.write(post);
                ///
            } else if (ct == typeof(array<mx>)) {
                /// binary transfer
                m.headers["Content-Length"] = str::from_integer(m.content.byte_len()); /// size() is part of mx object, should always return it size in its own unit, but a byte_size is there too
                return sc.write(m.content);
            } else {
                /// cstring
                assert(ct == typeof(char));
                m.headers["Content-Length"] = str::from_integer(m.content.byte_len());
                console.log("sending Content-Length of {0}", { m.headers["Content-Length"] });
                write_headers(sc);
                return sc.write(m.content);
            }
        }
        m.headers["Content-Length"] = 0;
        m.headers["Connection"]     = "keep-alive";
        return write_headers(sc);
    }

    str text() { return m.content.to_string(); }

    /// structure cookies into usable format
    map<str> cookies() {
        str  dec = uri::decode(m.headers["Set-Cookie"].grab());
        str  all = dec.split(",")[0];
        auto sep = all.split(";");
        auto res = map<str>();
        ///
        for (str &s: sep) {
            auto pair = s.split("="); assert(pair.len() >= 2);
            str   key = pair[0];
            if (!key)
                break;
            str   val = pair[1];
            res[key]  = val;
        }
        ///
        return res;
    }

    mx &operator[](symbol key) {
        return m.headers[key];
    }
    
    mx &header(mx key) { return m.headers[key]; }
};

///
/// high level server method (on top of listen)
/// you receive messages from clients through lambda; supports https
/// web sockets should support the same interface with some additions
async service(uri bind, lambda<message(message)> fn_process) {
    return sock::listen(bind, [fn_process](sock &sc) -> bool {
        bool close = false;
        for (close = false; !close;) {
            close  = true;
            array<char> msg = sc.read_until("\r\n", 4092); // this read 0 bytes, and issue ensued
            str param;
            ///
            if (!msg) break;
            
            /// its 2 chars off because read_until stops at the phrase
            uri req_uri = uri::parse(str(msg.data(), int(msg.len() - 2))); 
            ///
            if (req_uri) {
                console.log("request: {0}", { req_uri.resource() });
                message  req { req_uri };
                message  res = fn_process(req);
                close        = req.header("Connection") == "close";
                res.write(sc);
            }
        }
        return close;
    });
}

/// useful utility function here for general web requests; driven by the future
future request(uri url, map<mx> args) {
    return async(1, [url, args](auto p, int i) -> mx {
        map<mx> st_headers;
        mx      null_content;                   /// consider null static on var, assertions on its write by sequence on debug
        map<mx>         &a = *(map<mx> *)&args; /// so lame.  stop pretending that this cant change C++
        map<mx>    headers = a.count("headers") ? map<mx>(a["headers"]) : st_headers;
        mx         content = a.count("content") ?         a["content"]  : null_content;
        method        type = a.count("method")  ?  method(a["method"])  : method(method::get);
        uri          query = url.methodize(type);
        
        ///
        assert(query != method::undefined);
        console.log("(net) request: {0}", { query.string() });
        sock client = sock { sock::client, query };
        if (!client) return { };

        /// start request
        client.write("{0} {1} HTTP/1.1\r\n", {
            query.mtype().name(), client.query().string() });
        
        /// default headers
        array<field<mx>> defs = {
            { "User-Agent",      "core:net"             },
            { "Host",             client.query().host() },
            { "Accept-Language", "en-us"                },
            { "Accept-Encoding", "gzip, deflate, br"    },
            { "Accept",          "*/*"                  }
        };
        
        for (auto &d: defs)
            if (!headers(d.key))
                 headers[d.key] = d.value;
        
        message request { query, headers, content }; /// best to handle post creation within message
        request.write(client);
        
        /// read response, this is a message which constructs with a sock; does not store
        message response { client };
        
        /// close socket
        client.close();
        return response;
    });
}

future json(uri addr, map<mx> args, map<mx> headers) {
    lambda<void(mx)> success, failure;
    completer c  = { success, failure };
    ///
    request(addr, headers).then([ success, failure ](mx d) {
        (d.type() == typeof(map<mx>)) ?
            success(d) : failure(d);
        ///
    }).except([ failure ](mx d) {
        failure(d);
    });
    ///
    return future(c);
}

};