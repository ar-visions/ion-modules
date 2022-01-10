#pragma once
#include <dx/var.hpp>
#include <dx/nan.hpp>

template <typename T>
struct FlagsOf:io {
    uint32_t flags = 0;
    FlagsOf(nullptr_t = null)           { }
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
    Unit(const char *s = null) : Unit(str(s)) { }
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
    bool operator==(const char *s) { return type == s; }
    
    ///
    void assert_types(vec<str> &types, bool allow_none) {
        console.assertion((allow_none && !type) || types.index_of(type) >= 0, "unit not recognized");
    }
    ///
    operator str  &() { return type;  }
    operator real &() { return value; }
    
    operator var() {
        return Args {
            {"type", type},
            {"value", value}
        };
    }
};


struct Raw {
    std::vector<std::string> fields;
    std::vector<std::string> values;
};

typedef std::vector<Raw>                        Results;
typedef std::unordered_map<std::string, var>    Idents;
typedef std::unordered_map<std::string, Idents> ModelIdents;

struct ModelCache {
    Results       results;
    Idents        idents; /// lookup map needed for optimization
};

struct ModelContext {
    ModelMap      models;
    std::unordered_map<std::string, std::string> first_fields;
    std::unordered_map<std::string, ModelCache *> mcache;
    var           data;
};

struct DX {
    ModelContext &ctx;
    str           uri;
    DX(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
    virtual var record(var &view, int64_t primary_key = -1) = 0;
    virtual var lookup(var &view, int64_t primary_key) = 0;
    virtual bool reset(var &view) = 0;
};



