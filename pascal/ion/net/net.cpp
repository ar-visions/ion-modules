#include <net/net.hpp>

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

sock::sock(role r, uri bind) : sock() {
    /// if its release it must be secure
    symbol pers = "mx:net";
    static bool wolf;
    if (!wolf) {
#ifdef _WIN32
        static WSADATA wsa_data;
        static int wsa = WSAStartup(MAKEWORD(2,2), &wsa_data);
        if (wsa != 0) {
            printf("(sock) WSAStartup failed: %d\n", wsa);
            return;
        }
#endif
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

    size_t addr_sz       = sizeof(sockaddr_in);
    uint16_t  port       = (uint16_t)bind.port();
    str   &ip_addr       = bind.host(); /// todo: convert if not address
    i.query              = bind;
    i.sock_fd            = socket(AF_INET, SOCK_STREAM, 0);
    i.bound              = (sockaddr_in*)calloc(1, sizeof(sockaddr_in));
    i.bound->sin_family  = AF_INET;
    i.bound->sin_port    = htons(port);
   //i.bound->sin_len    = 4;
    inet_pton(AF_INET, ip_addr.cs(), &i.bound->sin_addr);

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

uri   sock::query()      const { return  i.query;      }
bool  sock::timeout_ms() const { return  i.timeout_ms; }
bool  sock::connected()  const { return  i.connected;  }
bool  sock::operator!()  const { return !i.connected;  }
sock::operator   bool()  const { return  i.connected;  }

///
void sock::set_timeout(i64 t) {
    i.timeout_ms = t;
}

sock sock::accept() {
    sock      sc = i;

#ifdef _WIN32
    using socklen_t = int;
    sc.i.bound->sin_addr.S_un.S_addr = INADDR_ANY;
#else
    sc.i.bound->sin_addr.s_addr = INADDR_ANY;
#endif

    socklen_t addr_sz = sizeof(sockaddr_in);
    sc.i.sock_fd = ::accept(i.sock_fd, (sockaddr*)&sc.i.bound, &addr_sz);
    printf("accepted: %i\n", sc.i.sock_fd);
    return sc.establish();
}

sock &sock::establish() {
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

int sock::read(u8 *v, size_t sz) {
    int    len = int(sz);
    int    rcv = wolfSSL_read(i.ssl, v, len);
    return rcv;
}

bool sock::read_sz(u8 *v, size_t sz) {
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

array<char> sock::read_until(str s, int max_len) {
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

bool sock::write(u8 *v, size_t sz) {
    assert(sz > 0);
    int      len = int(sz);
    int     wlen = wolfSSL_write(i.ssl, v, len);
    return (wlen == sz);
}

bool sock::write(array<char> v) {
    return write((u8*)v.data(), v.len());
}

bool sock::write(str s, array<mx> a) {
    str f = str::format(s.cs(), a);
    return write((u8*)f.cs(), f.len());
}

bool sock::write(str s) {
    return write((u8*)s.cs(), s.len());
}

/// for already string-like memory; this could do something with the type on mx
bool sock::write(mx &v) {
    return write((u8*)v.mem->origin, v.count() * v.type_size());
}

void sock::close() const {
#if _WIN32
    closesocket((SOCKET)i.sock_fd);
#else
    ::close(i.sock_fd);
#endif
    wolfSSL_CTX_free(i.ctx);
    //wolfSSL_Cleanup();
}

/// ---------------------------------
void sock::logging(void *ctx, int level, symbol file, int line, symbol str) {
    fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
}

/// ---------------------------------
sock sock::connect(uri u) {
    uri a = u.methodize(method::get);
    return sock(role::client, a);
}

/// listen on https using mbedtls
/// ---------------------------------
async sock::listen(uri url, lambda<bool(sock&)> fn) {
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







message::message(int server_code) : message() {
    m.code = mx(server_code);
}

message::message(symbol text) : message() {
    m.content = text;
    m.code    = 200;
}

message::message(str text) : message() {
    m.content = text;
    m.code    = 200;
}

message::message(path p, mx modified_since) : message() {
    m.content = p.read<str>();
    m.headers["Content-Type"] = p.mime_type();
    m.code    = 200;
}

message::message(mx content, map<mx> headers, uri query) : message() {
    m.query   = query;
    m.headers = headers;
    m.content = content; /// assuming the content isnt some error thing
    m.code    = 200;
}

message::message(uri url, map<mx> headers) : message() {
    m.query   = url;
    m.headers = headers;
}

/// important to note we do not keep holding onto this socket
message::message(sock sc) : message() {
    read_headers(sc);
    read_content(sc);
    m.code = m.headers["Status"];
}

uri message::query() { return m.query; }

bool message::read_headers(sock sc) {
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
bool message::read_content(sock sc) {
    str              te = "Transfer-Encoding";
    str              cl = "Content-Length";
    //cchar_t         *ce = "Content-Encoding";
    int            clen = int(m.headers.count(cl) ? str(m.headers[cl].grab()).integer_value() : -1);
    bool        chunked =     m.headers.count(te) &&    m.headers[te] == "chunked";
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
message message::query(uri server, map<mx> headers, mx content) {
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
message message::response(uri query, mx code, mx content, map<mx> headers) {
    message rs;
    message::members& r = rs;
    r.query    = { query, method::response };
    r.code     = code;
    r.headers  = headers;
    r.content  = content;
    return rs;
}

message::operator bool() {
    type_t ct = m.code.type();
    assert(ct == typeof(i32) || ct == typeof(str));
    return (m.query.mtype() != method::undefined) &&
            ((ct == typeof(i32) && int(m.code) >= 200 && int(m.code) < 300) ||
                (ct == typeof(str) && m.code.byte_len()));
}

bool message::operator!() { return !(operator bool()); }

bool message::printable_value(mx &v) {
    return v.type() != typeof(mx) || v.count() > 0;
}

symbol message::code_symbol(int code) {
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
bool message::write_headers(sock sc) {
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

bool message::write(sock sc) {
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

str message::text() { return m.content.to_string(); }

/// structure cookies into usable format
map<str> message::cookies() {
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

mx &message::operator[](symbol key) {
    return m.headers[key];
}

mx &message::header(mx key) { return m.headers[key]; }

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













