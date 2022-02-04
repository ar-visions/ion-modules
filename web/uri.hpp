#pragma once

/*
/// basically a [var reference] interface class
struct VInterface {
    var &data;
    VInterface(var &data): data(data) { }
};

/// heres something tht holds a value memory and a name, and a serialized value too.
struct MValue:VInterface {
    str   name;
    void *value;
    bool  sync = false;
    std::function<void()> del;
    
    ///
    var &nil() {
        static var n;
        return n;
    }
    
    template <typename T>
    MValue(str name, T value) : VInterface(nil()) {
        this->value = new T(value);
        this->name  = name;
    }
    
    /// no wonder its not getting called foo
    MValue(var &data, MValue &mv) : VInterface(data) {
        value = mv.value;
        name  = mv.name;
        
        /// transfer of ownership avoided with this
        //del = [&]() { delete (T *)this->value; };
    }
    
    /// use-case Member(str, T)
    template <typename T>
    MValue(var &data, str name, T value) : VInterface(data) {
        this->value = new T(value);
        this->name  = name;
    }
    
    MValue(const MValue &ref) : VInterface(ref.data) {
        value = ref.value;
        name  = ref.name;
    }
    
   ~MValue() {
       if (del)
           del();
   }
};

/// incorporate Member API into struct
/// port Component usage of Member into Struct-case
struct Struct:var {
    array<MValue> members;
    
    ///
    size_t count(std::string member) {
        size_t count = 0;
        for (auto &mv: members)
            if (mv.name == member)
                count++;
        return count;
    }
    
    ///
    template <typename T>
    T &get(str member) {
        assert(count(member) >= 1);
        for (auto &mv: members)
            if (mv.name == member)
                return *(T *)mv.value;
        ///
        assert(false);
        return *(T *)null;
    }
    
    ///
    template <typename T>
    T &set(str member, T &value) {
        assert(count(member) >= 1);
        for (auto &mv: members)
            if (mv.name == member)
                return (*(T *)mv.value = value);
        ///
        assert(false);
        return *(T *)null;
    }
    
    ///
    template <typename T>
    T &operator[](str m) {
        return get<T>(m);
    }
    
    ///
    Struct(std::initializer_list<MValue> v) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-variable"
        size_t reserve = v.size();
        #pragma GCC diagnostic pop
        
        ///
        members = array<MValue>(reserve);
        for (const MValue &mvalue:v) {
            
            MValue mv = MValue((var &)map()[mvalue.name], (MValue &)mvalue);
            members += mv;
        }
        ///
        bind_base(false);
    }
    
    ///
    virtual void bind() { }
};
*/

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
    
    T &operator()() {
        assert(value);
        return *value.get();
    }
    
    M<T> &operator=(const M<T> &ref) {
        if (this != &ref) { /// not bound at this point
            value = ref.value;
            assert(serialized && ref.serialized);
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

#define struct_shim(C)\
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
    virtual void bind() {
        std::cout << "Struct: bind\n";
    }
    
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
};

struct URI: Struct<URI> {
    enums(Method, Type, Undefined, Undefined, Response, Get,  Post,  Put, Delete);
    
    ///
    M<Method> type;
    M<str>    proto;
    M<str>    host;
    M<int>    port;
    M<str>    query;
    M<str>    resource;
    M<Map>    args;
    M<str>    version;
    
    ///
    void bind() { /// make a class table; the stubs just need a param
        member <str>    ("proto",    proto);
        member <Method> ("type",     type);
        member <str>    ("host",     host);
        member <int>    ("port",     port);
        member <str>    ("query",    query);
        member <str>    ("resource", resource);
        member <Map>    ("args",     args);
        member <str>    ("version",  version);
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
    bool    operator==        (Method::Type m) { return type() == m; }
    bool    operator!=        (Method::Type m) { return type() != m; }
            operator      bool()               { return type() != Method::Undefined; }
    
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
    //struct_shim(URI);
    
    

    URI():Struct() { bind_base(false); }
    URI(var &ref)  { t = ref.t; c = ref.c; m = ref.m; a = ref.a; bind_base(true); }
    inline URI &operator=(var ref) {
        if (this != &ref) { t = ref.t; c = ref.c; m = ref.m; a = ref.a; }
        if (!bound)       { bind_base(true); } else { assert(false); }
        return *this;
    }
    URI(const URI &ref) {
        ctype = ref.ctype;
        t = ref.t; c = ref.c; m = ref.m; a = ref.a;
        bind_base(false);
        for (size_t i = 0; i < members.size(); i++)
            members[i]->copy_fn(members[i], ((::array<Member *> &)ref.members)[i]);
    }

};


//template<> struct is_strable<URI> : std::true_type  {};
//bool is_def = d.select_first([&](defs &d) -> var { return var(bool(port == d.port && proto == d.proto)); });
