#pragma once
#include <web/socket.hpp>

/// message is trying to be all things to all bens
struct Message {
    URI    uri;
    var    code = 0;
    Map    headers;
    var    content;

    /// receiver
    Message(URI &uri, Socket sc); //

    /// request construction
    Message(URI &uri, Map &headers, var content); // make static.  not clear.

    /// response construction
    Message(var code, var content, Map headers = null): // make static.  not clear.
            code(code), headers(headers), content(content)
        { uri.type = URI::Method::Response; }

    Message(std::nullptr_t n = nullptr) { }
    
    Message(const Message &ref) :
             uri(ref.uri),
            code(ref.code),
         headers(ref.headers),
         content(ref.content) { }
    
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
        var vv  = d["uri"];
        URI u   = d["uri"];
        uri     = u;
        code    = d["code"];
        content = d["content"];
        headers = d["headers"];
    }

    bool write(Socket sc);
    bool write_headers(Socket sc);

    map<str, str> cookies();

    str text() { return str(content); }

    operator var() {
        return Map {
            {"uri",     uri},
            {"code",    code},
            {"content", content},
            {"headers", headers}
        };
    }

    operator bool() {
        assert(!code || code == Type::i32 || code == Type::Str);
        return ((*uri.type.value != URI::Method::Undefined) &&
                ((code == Type::i32 && int(code) >= 200 && int(code) < 300) ||
                 (code == Type::Str && code.size())));
    }

    bool read_headers(Socket sc);
    bool read_content(Socket sc);
    bool operator!() { return !(operator bool()); }
};


