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
struct StPair {
    str             member;
    str             value;
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
