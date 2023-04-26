#pragma once

#include <core/core.hpp>
#include <async/async.hpp>

enums(method, undefined, 
   "undefined, response, get, post, put, delete",
    undefined, response, get, post, put, del);

struct uri:mx {
protected:
    struct components {
        method  mtype; /// good to know where the uri comes from, in what context 
        str     proto; /// (what method, this is part of uri because method is 
        str     host;  /// Something that makes it uniquely bindable; losing that
        int     port;  /// makes you store it somewhere else and who knows where that might be)
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

    static memory *convert(memory *mem) { return parse(str(mem)); }

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
        method m { str(has_meth ? lc.cs() : cstr("get")) };
        str    u =  sp[has_meth ? 1 : 0];
        ra.mtype = m;

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

struct WOLFSSL_CTX;
struct WOLFSSL;
struct WOLFSSL_METHOD;
struct sockaddr_in;

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
        sockaddr_in*             bound;

        ~intern_t() {
            free(bound);
        }
    } &i;

    ctr(sock, mx, intern_t, i);

    ///
    uri       query() const;
    bool timeout_ms() const;
    bool  connected() const;
    bool  operator!() const;
    operator   bool() const;

    ///
    enum role {
        client,
        server
    };

    ///
    void set_timeout(i64 t);

    sock(role r, uri bind);

    sock accept();

    sock &establish();

    int read(u8 *v, size_t sz);

    bool read_sz(u8 *v, size_t sz);

    array<char> read_until(str s, int max_len);

    bool write(u8 *v, size_t sz);

    bool write(array<char> v);

    bool write(str s, array<mx> a);

    bool write(str s);

    /// for already string-like memory; this could do something with the type on mx
    bool write(mx &v);

    void close() const;

    /// ---------------------------------
    static void logging(void *ctx, int level, symbol file, int line, symbol str);
    
    /// ---------------------------------
    static sock connect(uri u);

    /// listen on https using mbedtls
    /// ---------------------------------
    static async listen(uri url, lambda<bool(sock&)> fn);
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

    message(int server_code);

    message(symbol text);

    message(str text);

    message(path p, mx modified_since = {});

    message(mx content, map<mx> headers = {}, uri query = {});

    message(uri url, map<mx> headers = {});

    /// important to note we do not keep holding onto this socket
    message(sock sc);

    uri query();

    bool read_headers(sock sc);

    ///
    bool read_content(sock sc);

    /// query/request construction
    static message query(uri server, map<mx> headers, mx content);

    /// response construction, uri is not needed
    static message response(uri query, mx code, mx content, map<mx> headers = null);

    explicit operator bool();

    bool operator!();
    
    bool printable_value(mx &v);

    static symbol code_symbol(int code);

    ///  code is in headers.
    bool write_headers(sock sc);

    bool write(sock sc);

    str text();

    /// structure cookies into usable format
    map<str> cookies();

    mx &operator[](symbol key);
    
    mx &header(mx key);
};

///
/// high level server method (on top of listen)
/// you receive messages from clients through lambda; supports https
/// web sockets should support the same interface with some additions
async service(uri bind, lambda<message(message)> fn_process);

/// useful utility function here for general web requests; driven by the future
future request(uri url, map<mx> args);

future json(uri addr, map<mx> args, map<mx> headers);
