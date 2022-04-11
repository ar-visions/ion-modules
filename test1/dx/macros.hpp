
/// a header that got through!
/// ye old C++ ways here. we dont even know what these terms are.  only they are colored glyphs.

#define ex_shim(C,E,D) \
    static Symbols symbols;\
    C(E t = D):ex<C>(t) { }\
    C(string s):ex<C>(D) { kind = resolve(s); }

/// ex-bound enums shorthand for most things
#define enums(C,E,D,ARGS...) \
    struct C:ex<C> {\
        static Symbols symbols;\
        enum E { ARGS };\
        C(E t = D):ex<C>(t)    { }\
        C(string s):ex<C>(D)   { kind = resolve(s); }\
    };\

#define io_shim(C,E) \
    C(std::nullptr_t n) : C()  { }                   \
    C(const C &ref)            { copy(ref);         }\
    C(var &d)                  { importer(d);       }\
    operator var()             { return exporter(); }\
    operator bool()  const     { return   E;        }\
    bool operator!() const     { return !(E);       }\
    C &operator=(const C &ref) {\
        if (this != &ref)\
            copy(ref);\
        return *this;\
    }

#define infinite_loop() { for (;;) { usleep(1000); } }