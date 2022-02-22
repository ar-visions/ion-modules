#pragma once
#include <dx/dx.hpp>

struct StQualifier {
    str             type;
    str             id;
    str             state; /// member to perform operation on (boolean, if not specified)
    str             oper;  /// if specified, use non-boolean operator
    str             value;
};

///
struct StTrans {
    enum Curve {
        None,
        Linear,
        EaseIn,
        EaseOut,
        CubicIn,
        CubicOut
    };
    Curve type;
    Unit duration;
    ///
    StTrans(str s) {
        auto sp = s.split();
        ///
        if (sp.size() == 2) {
            Unit u0   = sp[0];
            Unit u1   = sp[1];
            if (!u0.flags(Unit::Time) && u1.flags(Unit::Time))
                std::swap(u0, u1);
            duration  = u0;
            str  s1   = u1;
            /// an enumerable class fits the bill, especially if it integrates with Type
            static auto m = std::unordered_map<std::string, Curve> {
                { "linear",    Linear   },
                { "ease",      EaseIn   },
                { "ease-in",   EaseIn   },
                { "ease-out",  EaseOut  },
                { "cubic",     CubicIn  },
                { "cubic-in",  CubicIn  },
                { "cubic-out", CubicOut } /// throw all of them from W3C in. when you are feeling at ease
            };
            type = m.count(s1) ? m[s1] : None;
        } else if (sp.size() == 1) {
            type     = Linear;
            duration = sp[0];
        } else {
            type     = None;
            duration = Unit("0s");
        }
    }
    
    StTrans(std::nullptr_t n = null):type(None) { }
    
    real pos(real tf) const {
        real x = std::clamp(tf, 0.0, 1.0);
        switch (type) {
            case None:     x = 1.0;                    break;
            case Linear:                               break;
            case EaseIn:   x = x * x * x;              break;
            case EaseOut:  x = x * x * x;              break;
            case CubicIn:  x = x * x * x;              break;
            case CubicOut: x = 1 - std::pow((1-x), 3); break;
        };
        return x;
    }
    

    template <typename T>
    inline T operator()(T &fr, T &to, real tf) {
        if constexpr (uxTransitions<T>::enabled) {
            const real x = pos(tf);
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
    array<StQualifier>  quals;
    array<StPair>       pairs;
    array<ptr<StBlock>> blocks;
    double match(node *n);
    size_t score(node *n);
    
    StBlock() { }
    StBlock(StBlock &ref) { }
    
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
    static map<path_t, Style>            cache;
    static map<str, array<ptr<StBlock>>> members;
    array<ptr<StBlock>>                  root;
    ///
    Style(std::nullptr_t n = null)         { }
    Style(str &code);
   ~Style();
    ///
    void          cache_members();
    static void   init(path_t path = "style");
    static void   unload();
    static Style  load(path_t path);
    static Style *for_class(cchar_t *);
};
