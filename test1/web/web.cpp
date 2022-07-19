#include <web/web.hpp>

/// Useful utility function here for general web requests; driven by the Future
Future Web::request(URI url, Map args) {
    return async(1, [url=url, args=args](auto p, int i) -> var {
        Map     st_headers;
        var     null_content; /// consider null static on var, assertions on its write by sequence on debug
        Map             &a = *(Map *)&args;
        Map        headers = a.count("headers") ? a["headers"] : st_headers;
        var       &content = a.count("content") ? (var  &)a["content"] : null_content;
        URI::Method method = a.count("method")  ? URI::parse_method(str(a["method"])) : URI::Method::Get;
        URI            uri = url.methodize(method);
        
        ///
        assert(uri != URI::Method::Undefined);
        
        /// connect to server (secure or insecure depending on uri)
        //console.log("request: {0}", {uri.query});
        Socket sc = Socket::connect(uri);
        if (!sc)
            return null;

        /// start request
        str s_query = sc.uri.query;
        sc.write("{0} {1} HTTP/1.1\r\n", {uri.method_name(), uri.query});

        /// default headers
        struct Defs { str name, value; };
        array<Defs> vdefs = {
            { "User-Agent",      "ion"               },
            { "Host",             sc.uri.host        },
            { "Accept-Language", "en-us"             },
            { "Accept-Encoding", "gzip, deflate, br" },
            { "Accept",          "*/*"               }
          //{ "Content-Type",    "application/x-www-form-urlencoded" }
        };
        for (auto &d: vdefs)
            if (headers.count(d.name) == 0)
                headers[d.name] = var(d.value);
        Message  request = Message(uri, headers, content); /// best to handle post creation within Message
        request.write(sc);
        
        ///
        Message response = Message(uri, sc);
        
        /// close socket
        sc.close();
        return response;
    });
}

///
Future Web::json(URI url, Map args, Map headers) {
    std::function<void(var &)> s, f;
    Completer c = { s, f };
    Web::request(url, headers).then([s, f](var &d) {
        (d == Type::Map) ? s(d) : f(d);
    }).except([f](var &d) {
        f(d);
    });
    return Future(c);
}

///
/// high level server method (on top of listen)
/// you receive messages from clients through lambda; supports https
/// web sockets should support the same interface with some additions
async Web::server(URI url, std::function<Message(Message &)> fn) {
    return Socket::listen(url, [&, url=url, fn=fn](Socket sc) {
        for (bool close = false; !close;) {
            close       = true;
            array<char> m = sc.read_until("\r\n", 4092);
            str   param;
            ///
            if (!m) return false; /// question: is it normal for this to be the case? if its normal, false should not be an outcome.
            URI req_uri = URI::parse(str(m.data(), m.size() - 2)); /// its 2 chars off because read_until stops at the phrase
            ///
            if (req_uri) {
                Message req = Message(req_uri, sc);
                Message res = fn(req);
                close       = req.headers["Connection"] == var("close");
                res.write(sc);
            }
        }
        return true;
    });
}

///
/// structure cookies into usable format
map<str, str> Message::cookies() {
    cchar_t *n = "Set-Cookie";
    if (!headers.count(n))
        return null;
    str  dec = URI::decode(headers[n]);
    str  all = dec.split(",")[0];
    auto sep = all.split(";");
    auto res = map<str, str>();
    ///
    for (str &s: sep) {
        auto pair = s.split("=");   assert(pair.size() >= 2);
        str   key = pair[0];
        str   val = pair[1];
        res[key]  = val;
    }
    return res;
}
