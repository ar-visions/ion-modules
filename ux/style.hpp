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
    uint32_t flags;
    FlagsOf(T f) : flags(uint32_t(f)) { }
    bool operator()  (T c) { return (flags & c) == c; }
    void operator += (T f) { flags |=  f; }
    void operator -= (T f) { flags &= ~f; }
};

// not
struct Unit {
    enum Flags {
        Standard = 1,
        Metric   = 2,
        Percent  = 4
    };
    real v;
    FlagsOf<Flags> flags;
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
    StTrans(str s) {
        auto sp = s.split();
        // time = 0
        // type = 1
        str  t_combined = sp[0];
        
        
        // i dont have a complete list yet. plus i am in a funny mood.
        type = sp.size() >= 2 ? (sp[1] == "linear" ? Linear : (sp[1] == "cubic-out") ? CubicOut : (sp[1].substr(0, 5) == "cubic") ? CubicIn : None) : None;
    }
    StTrans(nullptr_t n = null) { }
    template <typename T>
    inline T operator()(T &fr, T &to, real tf) {
        real x = std::clamp(tf, 0.0, 1.0);
        switch (type) {
            case None:     x = 1.0;                        break;
            case Linear:                                   break;
            case CubicIn:  x = x * x * x;                  break;
            case CubicOut: x = 1.0 - std::pow(1.0 - x, 3); break;
        };
        const real i = 1.0 - x;
        return (fr * i) + (to * x);
    }
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
