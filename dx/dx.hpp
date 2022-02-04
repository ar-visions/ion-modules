#pragma once
#include <dx/type.hpp>
#include <dx/nan.hpp>

enum Role {
    Client,
    Server
};

template <typename T>
struct FlagsOf:io {
    uint32_t flags = 0;
    FlagsOf(std::nullptr_t = null) { }
    FlagsOf(std::initializer_list<T> l) {
        for (auto i:l)
            flags |= i;
    }
    FlagsOf(T f) : flags(uint32_t(f))   { }
    bool operator()  (T c) { return (flags & c) == c; }
    void operator += (T f) { flags |=  f; }
    void operator -= (T f) { flags &= ~f; }
    operator var() { return flags; }
    FlagsOf(var &v) {
    }
};

struct Unit:io {
    enum UFlag {
        Standard = 1,
        Metric   = 2,
        Percent  = 4,
        Time     = 8,
        Distance = 16
    };
    
    ///
    static std::unordered_map<str, int> u_flags;
    str            type   = null;
    real           value  = dx::nan<real>();
    int64_t        millis = 0;
    FlagsOf<UFlag> flags  = null;
    
    /// 
    void init() {
        if (type && !std::isnan(value)) {
            if (type == "s" || type == "ms") {
                millis = int64_t(type == "s" ? value * 1000.0 : value);
                flags += Time;
            }
        }
        if (type && u_flags.count(type))
            flags += UFlag(u_flags[type]);
    }
    
    ///
    Unit(cchar_t *s = null) : Unit(str(s)) { }
    Unit(var &v) {
        value  = v["value"];
        type   = v["type"];
        init();
    }
    
    ///
    Unit(str s) {
        int  i = s.index_of(str::Numeric);
        int  a = s.index_of(str::Alpha);
        if (i >= 0 || a >= 0) {
            if (a == -1) {
                value = s.substr(i).trim().real();
            } else {
                if (a > i) {
                    if (i >= 0)
                        value = s.substr(0, a).trim().real();
                    type  = s.substr(a).trim().to_lower();
                } else {
                    type  = s.substr(0, a).trim().to_lower();
                    value = s.substr(a).trim().real();
                }
            }
        }
        init();
    }
    
    ///
    bool operator==(cchar_t *s) { return type == s; }
    
    ///
    void assert_types(array<str> &types, bool allow_none) {
        console.assertion((allow_none && !type) || types.index_of(type) >= 0, "unit not recognized");
    }
    ///
    operator str  &() { return type;  }
    operator real &() { return value; }
    
    operator var() {
        return Map {
            {"type",  var(type)},
            {"value", var(value)}
        };
    }
};

struct Member;
typedef std::function<void(Member &, var &)>  MFn;
typedef std::function< var(Member &)>         MFnGet;
typedef std::function<bool(Member &)>         MFnBool;
typedef std::function<void(Member &)>         MFnVoid;
typedef std::function<void(Member &, void *)> MFnArb;
struct StTrans;

struct Member {
    enum MType { /// multi-planetary enum.
        Undefined,
        Stationary,
        Intern,
        Extern,
        Configurable = Intern
    };
    std::function<void(Member *, Member *)> copy_fn;
    var *serialized = null;
    var &nil() {
        static var n;
        return n;
    }
    
    ///
    Type     type       =  Type::Undefined;
    MType    member     = MType::Undefined;
    str      name       = null;
    size_t   cache      = 0;
    size_t   peer_cache = 0xffffffff;
    Member  *bound_to   = null;
    void    *arg        = null; /// user-defined arg (used with node binding in ux)
    ///
    virtual void *ptr()        { return null; }
    virtual bool  state_bool() {
        assert(false);
        return bool(false);
    }
    ///
    /// once per type
    struct Lambdas {
        MFn      var_set;
        MFnGet   var_get;
        MFnBool  get_bool;
        MFnVoid  compute_style;
        MFnArb   type_set;
        MFnVoid  unset;
    } *fn = null;
    ///
    
    ///
    virtual void operator=(const str &s) {
        var v = (str &)s;
        if (fn && fn->var_set)
            fn->var_set(*this, v);
    }
    ///
    void operator=(Member &v);
    ///
    virtual bool operator==(Member &m) {
        return bound_to == &m && peer_cache == m.peer_cache;
    }
    
    virtual void style_value_set(void *ptr, StTrans *) { }
};



struct node;

struct Raw {
    std::vector<std::string> fields;
    std::vector<std::string> values;
};

typedef std::vector<Raw>                        Results;
typedef std::unordered_map<std::string, var>    Idents;
typedef std::unordered_map<std::string, Idents> ModelIdents;

struct ModelCache {
    Results       results;
    Idents        idents; /// lookup fmap needed for optimization
};

struct ModelContext {
    ModelMap      models;
    std::unordered_map<std::string, std::string> first_fields;
    std::unordered_map<std::string, ModelCache *> mcache;
    var           data;
};

struct Adapter { // this was named DX, it needs a different name than this, though.  DX is the API end of an app, its data experience or us
    ModelContext &ctx;
    str           uri;
    Adapter(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
    virtual var record(var &view, int64_t primary_key = -1) = 0;
    virtual var lookup(var &view, int64_t primary_key) = 0;
    virtual bool reset(var &view) = 0;
};

#include <dx/path.hpp>

#define infinite_loop() { for (;;) { usleep(1000); } }

