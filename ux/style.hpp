#pragma once
#include <dx/dx.hpp>

struct StQualifier {
    str             type;
    str             id;
    str             state; /// member to perform operation on (boolean, if not specified)
    str             oper;  /// if specified, use non-boolean operator
    str             value;
};

template <typename T>
struct FlagsOf {
    uint32_t flags = 0;
    FlagsOf(nullptr_t = null)         { }
    FlagsOf(T f) : flags(uint32_t(f)) { }
    bool operator()  (T c) { return (flags & c) == c; }
    void operator += (T f) { flags |=  f; }
    void operator -= (T f) { flags &= ~f; }
};

struct Unit {
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
    real           value  = std::numeric_limits<real>::quiet_NaN();
    int64_t        millis = 0;
    FlagsOf<UFlag> flags  = null;
    ///
    Unit(nullptr_t = null) { }
    Unit(str s) {
        int  i = s.index_of(str::Numeric);
        int  a = s.index_of(str::Alpha);
        ///
        console.assertion(i >= 0 || a >= 0, "required symbol or value");
        ///
        if (a == -1) {
            value = s.substr(i).trim().real();
        } else {
            if (a > i) {
                if (i >= 0)
                    value = s.substr(0, a).trim().real();
                type  = s.substr(a).trim();
            } else {
                type  = s.substr(0, a).trim();
                value = s.substr(a).trim().real();
            }
        }
        if (type && !std::isnan(value)) {
            std::unordered_map<std::string, real> m = {
                {"s",  value * 1000.0},
                {"ms", value},
                {"us", value / 1000.0}, ///
            };
            millis = int64_t(type.map<real>(m)); // fault when we use an unsupported type at the moment
            flags += Time;
        }
        if (type && u_flags.count(type))
            flags += UFlag(u_flags[type]);
    }
    ///
    void assert_types(vec<str> &types, bool allow_none) {
        console.assertion((allow_none && !type) || types.index_of(type) >= 0, "unit not recognized");
    }
    ///
    operator str  &() { return type;  }
    operator real &() { return value; }
};


///
struct StTrans {
    enum Type {
        None,
        Linear,
        CubicIn,
        CubicOut
    };
    Type type;
    Unit duration;
    ///
    StTrans(str s) {
        auto sp = s.split();
        ///
        if (sp.size() == 2) {
            Unit u0   = sp[0];
            Unit u1   = sp[1];
            /// 'just work'
            if (!u0.flags(Unit::Time) && u1.flags(Unit::Time))
                std::swap(u0, u1);
            duration  = u0;
            str  s1   = u1;
            static std::unordered_map<std::string, Type> m = {
                { "linear",    Linear   },
                { "cubic-in",  CubicIn  },
                { "cubic-out", CubicOut }};
            type = s1.map<Type>(None, m);
        } else if (sp.size() == 1) {
            type     = Linear;
            duration = sp[0];
        } else {
            type     = None;
            duration = Unit("0s");
        }
    }
    ///
    StTrans(nullptr_t n = null) { }
    ///
    template <typename T>
    inline T operator()(T &fr, T &to, real tf) {
        if constexpr (uxTransitions<T>::enabled) {
            real x = std::clamp(tf, 0.0, 1.0);
            switch (type) {
                case None:     x = 1.0;                    break;
                case Linear:                               break;
                case CubicIn:  x = x * x * x;              break;
                case CubicOut: x = 1 - std::pow((1-x), 3); break;
            };
            const real i = 1.0 - x;
            return (fr * i) + (to * x);
        }
        return to;
    }
    ///
    operator  bool() { return type != None; }
    bool operator!() { return type == None; }
};
///
struct StPair {
    str             member;
    str             value;
    StTrans         trans;
    Instances       instances; /// typed instances of the value, hash_t is Type
};
///
struct StBlock {
    StBlock          *parent = null;
    vec<StQualifier>  quals;
    vec<StPair>       pairs;
    vec<ptr<StBlock>> blocks; // one idea was to make a container for smart pointer allocated objects.  if they use the info standard they may.
    double match(node *n);
    size_t score(node *n);
    
    StBlock() { }
    StBlock(StBlock &ref) {
        int test = 0;
        test++;
    }
    
    size_t count(str &s) {
        for (auto &p:pairs)
            if (p.member == s)
                return 1;
        return 0;
    }
    StPair *pair(str &s) {
        for (auto &p:pairs)
            if (p.member == s)
                return &p;
        return null;
    }
};

struct Style {
    static map<path_t, Style>          cache;
    static map<str, vec<ptr<StBlock>>> members;
    vec<ptr<StBlock>>                  root;
    ///
    Style(nullptr_t n = null)         { }
    Style(str &code);
   ~Style();
    ///
    void          cache_members();
    static void   init(path_t path = "style");
    static void   unload();
    static Style  load(path_t path);
    static Style *for_class(const char *);
};
