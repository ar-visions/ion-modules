#pragma once
#include <functional>

template <typename T>
struct M:Member {
    typedef T value_type;
    
    ///
    std::shared_ptr<T> value; /// Member gives us types, order, serialization and the shared_ptr gives us memory management
    bool sync  = false;
    
    ///
    void operator=(T v) {
        *value = v;
        sync   = false;
    }
    
    /// no need to reserialize when possible
    operator var &() {
        if (!sync) {
            var &r = *serialized;
            r      = var(*value.get());
            sync   = true;
        }
        return *serialized;
    }
    
    /// construct from var
    M(var &v) {
        exit(0);
        value      = std::shared_ptr<T>(new T(v)); // this is a T *, dont get it
        serialized = v.ptr();  // this definitely cant happen
        sync       = true;
    }
    
    /// get reference to value memory (note this is not how we would assign Member from another Member)
    operator T &() {
        assert(value);
        return *value.get();
    }
    ///
    T &operator()() {
        assert(value);
        return *value.get();
    }
    ///
    template <typename R, typename A>
    R operator()(A a) { return (*(std::function<R(A)> *)value)(a); }
    ///
    template <typename R, typename A, typename B>
    R operator()(A a, B b) { return (*(std::function<R(A,B)> *)value)(a, b); }
    ///
    M<T> &operator=(const M<T> &ref) {
        if (this != &ref) { /// not bound at this point
            value = ref.value;
            //assert(serialized && ref.serialized);
            //*serialized = *ref.serialized; /// not technically needed yet, only when the var is requested
            sync = false;
        }
        return *this;
    }
    /// will likely need specialization of a specific type named VRef or Ref, just var:Ref, thats all
    M<T> &operator=(var &ref) {
        if (this  != &ref) {
            *value = T(ref);
            //*serialized = *ref; /// not technically needed yet, only when the var is requested
            sync = false;
        }
        return *this;
    }
    ///
    M() { value = null; }
   ~M() { }
};

template <typename F>
struct Lambda:M<std::function<F>> {
    typedef std::function<F> fun;
    typedef typename function_traits<fun>::result_type rtype;
    static const size_t arg_count = function_traits<fun>::arg_count;
    rtype last_result;
};

///
#define struct_shim(C)\
bool operator==(C &b) { return Struct::operator==(b); }\
C():Struct() { bind_base(false); }\
C(var &ref)  { t = ref.t; c = ref.c; m = ref.m; a = ref.a; bind_base(true); }\
inline C &operator=(var ref) {\
    if (this != &ref) { t = ref.t; c = ref.c; m = ref.m; a = ref.a; }\
    if (!bound)       { bind_base(true); } else { assert(false); }\
    return *this;\
}\
C(const C &ref):C() {\
    ctype = ref.ctype;\
    t = ref.t; c = ref.c; m = ref.m; a = ref.a;\
    bind_base(false);\
    for (size_t i = 0; i < members.size(); i++)\
        members[i]->copy_fn(members[i], ((::array<Member *> &)ref.members)[i]);\
}

template <typename C>
struct Struct:var {
    Id<C>           ctype;
  ::array<Member *> members;
    bool            bound = false;
    
    Struct() { }
    Struct(const Struct &ref) {
        std::string code_name = ref.ctype.code_name;
        ctype = ref.ctype;
        for (size_t i = 0; i < members.size(); i++)
            members[i]->copy_fn(members[i], ((::array<Member *> &)ref)[i]);
    }
    
    Struct& operator=(const Struct &ref) {
        if (this != &ref) {
            ctype   = ref.ctype;
            auto &r = (Struct &)ref;
            for (size_t i = 0; i < members.size(); i++)
                members[i]->copy_fn(members[i], r.members[i]);
        }
        return *this;
    }
    
    ///
    template <typename T>
    void member(cchar_t *name, M<T> &member) {
        if (!member.value)
             member.value = std::shared_ptr<T>(new T());
        ///
        member.copy_fn    = [](Member *dst, Member *src) { *(M<T> *)dst = *(M<T> *)src; };
        member.name       = string(name);
      //array()          += &member; # simpler but less secure and more complicated to endure internal to var, so not.
        members          += &member;
        map()[name]       = var(null);
    }

    ///
    template <typename F>
    void _lambda(cchar_t *name, Lambda<F> &lambda, std::function<F> fn) {
        typedef typename Lambda<F>::fun   Lfun;
        typedef typename Lambda<F>::rtype Rtype;
        ///
        if (!lambda.value) {
             lambda.value       = std::shared_ptr<Lfun>(new Lfun(fn));
             lambda.last_result = Rtype(); /// namco music
        }
        ///
        lambda.copy_fn    = [](Member *dst, Member *src) {
            console.fault("illegal method assignment, we dont set these dynamically at runtime by design.");
        };
        /// auto convert the types or be strict about no conversion other than atype[n] to atype[n]. strict is default
        ///
        lambda.name       = string(name);
      //array()          += &member; # simpler but less secure and more complicated to endure internal to var, so not.
      //members          += &lambda; # will need a separate container for introspection
        Fn var_fn = [la=&lambda] (var  &args) {
            Rtype         &r =  la->last_result; /// return memory
            Lfun          &f = *la->value;       /// lambda memory std::function<F>(fn)
            ::array<var>  &a =  args.array();    /// each request should allocate an args to prevent realloc
            ///
            if      constexpr (Lambda<F>::arg_count == 0)
                r = f();
            if      constexpr (Lambda<F>::arg_count == 1)
                r = f(a[0]);
            else if constexpr (Lambda<F>::arg_count == 2)
                r = f(a[0], a[1]);
            else if constexpr (Lambda<F>::arg_count == 3)
                r = f(a[0], a[1], a[2]);
            else if constexpr (Lambda<F>::arg_count == 4)
                r = f(a[0], a[1], a[2], a[3]);
            else if constexpr (Lambda<F>::arg_count == 5)
                r = f(a[0], a[1], a[2], a[3], a[4]);
            else if constexpr (Lambda<F>::arg_count == 6)
                r = f(a[0], a[1], a[2], a[3], a[4], a[5]);
            else if constexpr (Lambda<F>::arg_count == 7)
                r = f(a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
            else
                console.fault("arg count not supported");
        };
        map()[name] = var_fn; /// object holds onto var conversion
    }
    ///
    virtual void bind() { std::cout << "Struct: bind\n"; }
    ///
    void bind_base(bool read_var) {
        bind();
        auto &m = map();
        for (size_t i = 0; i < members.size(); i++) {
            Member &member    = *members[i];
            member.serialized =  m[member.name].ptr();
        }
        bound = true;
    }
    bool operator==(Struct &b) {
        size_t a_sz =   members.size();
        size_t b_sz = b.members.size();
        bool   i_sz = a_sz == b_sz;
        for (size_t i = 0; i_sz && i < a_sz; i++) {
            Member &am = *  members[i];
            Member &bm = *b.members[i];
            if (am.shared != bm.shared)
                return false;
        }
        return i_sz;
    }
    
    virtual ~Struct() { }
};

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
    
    int test_func(string s) {
        return s.integer() + 1;
    }
    
    template <typename L, typename TT>
    void func(string n, Lambda<L> &l, std::function<L> fn) {
        _lambda<L> (n.cstr(), l, fn);
    }
    
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
        
        // [ ] support lambda service
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
    
    ///
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
