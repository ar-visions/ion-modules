#pragma once
#include <functional>

///
struct URI: Struct<URI> {
    enums(Method, Type, Undefined, Undefined, Response, Get,  Post,  Put, Delete);
    
    M<Method> type;
    M<str>    proto;
    M<str>    host;
    M<int>    port;
    M<str>    query;
    M<str>    resource;
    M<Map>    args;
    M<str>    version;
    Lambda<int(string)> test_lambda;
    Lambda<int(string)> test_lambda2;
    
    int test_func(string s) { return s.integer() + 1; }
    
    template <typename L, typename TT>
    void func(string n, Lambda<L> &l, std::function<L> fn) { _lambda<L> (n.cstr(), l, fn); }
    
    /// same interface for either lambdas or methods
    template <typename L, typename TT>
    void func(string n, Lambda<L> &l, TT &&m) {
        ///
        const size_t args = function_traits<std::function<L>>::arg_count;
        if constexpr (std::is_member_function_pointer<TT>()) {
            if      constexpr (args == 0)
                _lambda<L> (n.cstr(), l, std::bind(m, this));
            else if constexpr (args == 1)
                _lambda<L> (n.cstr(), l, std::bind(m, this, std::placeholders::_1));
            else if constexpr (args == 2)
                _lambda<L> (n.cstr(), l, std::bind(m, this, std::placeholders::_1, std::placeholders::_2));
            else if constexpr (args == 3)
                _lambda<L> (n.cstr(), l, std::bind(m, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            else if constexpr (args == 4)
                _lambda<L> (n.cstr(), l, std::bind(m, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            else if constexpr (args == 5)
                _lambda<L> (n.cstr(), l, std::bind(m, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        } else {
            _lambda<L> (n.cstr(), l, m);
        }
    }
    
    ///
    void bind() {
        member <str>    ("proto",    proto);
        member <Method> ("type",     type);
        member <str>    ("host",     host);
        member <int>    ("port",     port);
        member <str>    ("query",    query);
        member <str>    ("resource", resource);
        member <Map>    ("args",     args);
        member <str>    ("version",  version);
        ///
        func <int(string)> ("test_lambda",  test_lambda,  &URI::test_func);
        func <int(string)> ("test_lambda2", test_lambda2, [](string s) -> int { return s.integer() + 1; });
    }
    
    ///
    static str encode_fields(var &content) {
        str post;
        if (content)
            for (auto &[key,val]: content.map()) {
                if (post)
                    post += "&";
                post += URI::encode(key);
                post += "=";
                post += URI::encode(val);
            }
        return post;
    }

    URI(str s) {
        bind_base(false);
        *this = parse(s);
    }
    
    ///
    URI(cchar_t *s) : URI(str(s)) { }
    
    ///
    operator str () {
        struct def  {
            cchar_t *proto;
            int      port;
        };
      ::array<def> defs = {{ "https", 443 }, { "http", 80 }};
        bool     is_def = defs.select_first<bool> ([&](def &d) { return port() == d.port && proto() == d.proto; });
        str      s_port = is_def ? "" : var::format(":{0}", { port }), s_fields;
        if (args().size()) {
            var  v_args = args; /// it will be useful as an exercise to see if we can deprecate Map in favor of var; data structure is there, we just need no ambiguity in cases
            s_fields    = var::format("?{0}", { encode_fields(v_args) });
        }
        // so proto.name is "", cant be.  must not be initialized or corrupted
        return var::format("{0}://{1}{2}{3}{4}", { proto, host, s_port, query, s_fields });
    }

    URI methodize(Method m) const {
        URI uri  = *this;
        uri.type = m;
        return uri;
    }
    
    static str          decode(str e);
    static str          encode(str s);
    static Method parse_method(str s);
    str            method_name() { return type().symbol(); }
    ///
    bool    operator==        (typename Method::Type m) { return type() == m; }
    bool    operator!=        (typename Method::Type m) { return type() != m; }
            operator      bool()                        { return type() != Method::Undefined; }
    
    /// can contain: GET/PUT/POST/DELETE uri [PROTO] (defaults to GET)
    static URI parse(str uri, URI *ctx = null) {
        auto   sp       = uri.split(" ");
        URI    ret;          /// this should be calling bind.
        bool   has_meth = sp.size() > 1;
        Method m        = parse_method(has_meth ? sp[0] : "GET");
        str    u        = sp[has_meth ? 1 : 0];
        ret.type        = m; /// this member is not bound still, . oops.
        int iproto      = u.index_of("://");
        if (iproto >= 0) {
            str p       = u.substr(0, iproto);
            u           = u.substr(iproto + 3);
            int ihost   = u.index_of("/");
            ret.proto   = p;
            ret.query   = ihost >= 0 ? u.substr(ihost)    : str("/");
            str  h      = ihost >= 0 ? u.substr(0, ihost) : u;
            int ih      = h.index_of(":");
            u           = ret.query;
            if (ih > 0) {
                ret.host  = h.substr(0, ih);
                ret.port  = h.substr(ih + 1).integer();
            } else {
                ret.host  = h;
                ret.port  = p == "https" ? 443 : 80;
            }
        } else {
            ret.proto     = ctx ? ctx->proto() : "";
            ret.host      = ctx ? ctx->host()  : "";
            ret.port      = ctx ? ctx->port()  : 0;
            ret.query     = u;
        }
        /// parse resource and query
        auto iq           = u.index_of("?");
        if (iq > 0) {
            ret.resource  = decode(u.substr(0, iq));
            str q         = u.substr(iq + 1);
            ret.args      = Map(); // arg=1,arg=2
        } else
            ret.resource  = decode(u);
        ///
        if (sp.size() >= 3)
            ret.version   = sp[2];
        return ret;
    }
    
    ///
    struct_shim(URI);
};


//template<> struct is_strable<URI> : std::true_type  {};
//bool is_def = d.select_first([&](defs &d) -> var { return var(bool(port == d.port && proto == d.proto)); });
