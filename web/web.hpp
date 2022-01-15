#pragma once
#include <dx/dx.hpp>
#include <thread>
#include <mutex>

struct URI {
    enum Method { Undefined, Response, Get,  Post,  Put, Delete };
    Method type = Undefined;
    str    proto;
    str    host;
    int    port;
    str    query;
    str    resource;
    Args   args;
    str    version;
    
    operator var() {
        return Args {{
            {"type",     int(type)},
            {"proto",    proto},
            {"host",     host},
            {"port",     port},
            {"query",    query},
            {"resource", resource},
            {"args",     args},
            {"version",  version},
        }};
    }
    URI(var &d) {
        type     = Method(int(d["type"]));
        proto    = d["proto"];
        host     = d["host"];
        port     = d["port"];
        query    = d["query"];
        resource = d["resource"];
        args     = d["args"];
        version  = d["version"];
    }
    
    URI(std::nullptr_t n = nullptr) { }
    
    static map<str, URI::Method> mmap;
    
    static str decode(str e);
    static str encode(str s);
    static Method parse_method(str s);
    
    str method_name() {
        for (auto &[k,v]: mmap) {
            if (v == type)
                return k;
        }
        return null;
    }
    
    bool operator==(Method m) { return type == m; }
    bool operator!=(Method m) { return type != m; }
    
    /// must contain: GET/PUT/POST/DELETE uri [PROTO]
    static URI parse(str uri, URI *ctx = null) {
        auto sp = uri.split(" ");
        if (sp.size() < 2)
            return {};
        
        URI    ret;
        Method m      = parse_method(sp[0]);
        str    u      = sp[1];
        ret.type      = m;
        int iproto    = u.index_of("://");
        if (iproto >= 0) {
            str p     = u.substr(0, iproto);
            u         = u.substr(iproto + 3);
            int ihost = u.index_of("/");
            ret.proto = p;
            ret.query = ihost >= 0 ? u.substr(ihost)    : str("/");
            str  h    = ihost >= 0 ? u.substr(0, ihost) : u;
            int ih    = h.index_of(":");
            u         = ret.query;
            if (ih > 0) {
                ret.host  = h.substr(0, ih);
                ret.port  = h.substr(ih + 1).integer();
            } else {
                ret.host  = h;
                ret.port  = p == "https" ? 443 : 80;
            }
        } else {
            ret.proto     = ctx ? ctx->proto : "";
            ret.host      = ctx ? ctx->host  : "";
            ret.port      = ctx ? ctx->port  : 0;
            ret.query     = u;
        }

        // parse resource and query
        auto iq           = u.index_of("?");
        if (iq > 0) {
            ret.resource  = decode(u.substr(0, iq));
            str q         = u.substr(iq + 1);
            ret.args      = {}; // arg=1,arg=2
        } else
            ret.resource  = decode(u);
        
        if (sp.size() >= 3)
            ret.version = sp[2];
        
        return ret;
    }
};

struct SocketInternal;
struct Socket {
    URI uri;
    std::shared_ptr<SocketInternal> intern;
    bool        is_secure();
    Socket(std::nullptr_t n = nullptr);
    Socket(const Socket &ref);
    Socket(bool secure, bool listen);
   ~Socket();
    Socket &operator=(const Socket &ref);
    static Socket connect(str uri, bool trusted_only = true);
    static Async   listen(str uri, std::function<void(Socket)> fn);
    static void   logging(void *ctx, int level, const char *file, int line, const char *str);
    bool            write(const char *v, size_t sz, int flags);
    bool            write(str s, std::vector<var> a = {});
    bool            write(vec<char> &v);
    bool            write(var &d);
    bool             read(const char *v, size_t sz);
    int          read_raw(const char *v, size_t sz);
    vec<char>  read_until(str s, int max_len);
    void            close() const;
    
    bool  operator!() const { return !connected; };
    operator   bool() const { return  connected; };
    
protected:
    bool connected = false;
};

struct Web {
protected:
    static var    read_content(Socket sc, Args &headers);
    static Args    read_headers(Socket sc);
    static bool   write_headers(Socket sc, Args &headers);
    static var    content_data(Socket sc, var &c, Args &headers);
    static str          cookie(var &result);
    
public:
    
    struct Message {
        URI    uri;
        int    code = 0;
        var    content;
        Args   headers;
        
        Message(int code, var content = null, Args headers = null):
                code(code), content(content), headers(headers) {
            uri.type = URI::Response;
        }
        
        Message(std::nullptr_t n = nullptr) { }
        
        Message(const Message &ref) :
                uri(ref.uri),
               code(ref.code),
            content(ref.content),
            headers(ref.headers) { }
        
        Message &operator=(const Message &ref) {
            if (&ref != this) {
                uri     = ref.uri;
                code    = ref.code;
                content = ref.content;
                headers = ref.headers;
            }
            return *this;
        }
        Message(var &d) {
            uri     = d["uri"];
            code    = d["code"];
            content = d["content"];
            headers = d["headers"];
        }
        map<str, str> cookies();
        str text() {
            return str(content);
        }
        operator var() {
            return Args {
                {"uri",     uri},
                {"code",    int(code)},
                {"content", content},
                {"headers", headers}
            };
        }
        operator bool() {
            return uri.type != URI::Undefined && code >= 200 && code < 300;
        }
        bool operator!() {
            return !(operator bool());
        }
    };
    static Async         server(str uri, std::function<Message(Message &)> fn);
    static Future       request(str uri, Args headers = {});
    static Future          json(str uri, Args args = {}, Args headers = {});
};
