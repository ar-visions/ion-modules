#pragma once

struct Unit:io {
    enum Attrib {
        Standard,
        Metric,
        Percent,
        Time,
        Distance
    };
    
    ///
    static std::unordered_map<str, int> u_flags;
    str             type   = null;
    real            value  = dx::nan<real>();
    int64_t         millis = 0;
    FlagsOf<Attrib> flags  = null; /// FlagsOf makes flags out of most of the things
    
    ///
    void init() {
        if (type && !std::isnan(value)) {
            if (type == "s" || type == "ms") {
                millis = int64_t(type == "s" ? value * 1000.0 : value);
                flags += Time;
            }
        }
        if (type && u_flags.count(type))
            flags += Attrib(u_flags[type]);
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

