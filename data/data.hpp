#pragma once
#define _USE_MATH_DEFINES
#include <iostream>
#include <map>
#include <stack>
#include <array>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <limits>
#include <cmath>
#include <math.h>
#include <functional>
#include <assert.h>
#include <cstring>
#include <unistd.h>
#include <random>

// nullptr verbose, incorrect (nullptr_t is not constrained to pointers at all)
static const nullptr_t null = nullptr;

static inline
std::chrono::milliseconds ticks() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

template <class T>
static inline T interp(T from, T to, T f) {
    return from * (T(1) - f) + to * f;
}

template <typename T>
static inline T *allocate(size_t count, size_t align = 16) {
    size_t sz = count * sizeof(T);
    return (T *)aligned_alloc(align, (sz + (align - 1)) & ~(align - 1));
}

template <typename T>
static inline void memclear(T *dst, size_t count = 1, int value = 0) {
    memset(dst, value, count * sizeof(T));
}

template <class T>
static inline void memclear(T &dst, size_t count = 1, int value = 0) {
    memset(&dst, value, count * sizeof(T));
}

template <typename T>
static inline void memcopy(T *dst, T *src, size_t count = 1) {
    memcpy(dst, src, count * sizeof(T));
}

#include <data/map.hpp>

typedef std::filesystem::path                       path_t;

struct node;
struct var;
typedef std::function<void(var &)>                 Fn;
typedef std::function<var(var &)>                 FnFilter;
typedef std::function<var(var &, std::string &)>  FnFilterMap;
typedef std::function<var(var &, size_t)>         FnFilterArray;
typedef std::function<void(var &, std::string &)>  FnEach;
typedef std::function<void(var &, size_t)>         FnArrayEach;

struct VoidRef {
    void *v;
    VoidRef(void *v) : v(v) { }
};

struct str;

struct var {
    enum Type {
        Undefined,
        i8,  ui8,
        i16, ui16,
        i32, ui32,
        i64, ui64,
        f32, f64,
        Bool, Str, Map, Array, Ref, Lambda, Filter, Any
    } t = Undefined,
      c = Undefined;

    enum Flags {
        Encode  = 1,
        Compact = 2
    };
    
    struct TypeFlags {
        var::Type t;
        int flags;
    };
    
    enum Format {
        Binary,
        Json
    };
    
    static size_t type_size(var::Type t);
    
    union u {
        bool    vb;
        int8_t  vi8;
        int16_t vi16;
        int32_t vi32;
        int64_t vi64;
        float   vf32;
        double  vf64;
    };
    u                   n_value = { 0 };
    Fn                       fn;
    FnFilter                 ff;
    std::shared_ptr<uint8_t>                  d = nullptr;
    std::shared_ptr<::map<std::string, var>> m = nullptr;
    std::shared_ptr<std::vector<var>>        a = nullptr;
    std::shared_ptr<std::string>              s = nullptr;
    u                                        *n = nullptr; // we CAN utilize n.
    
    int                   flags = 0;
    std::vector<int>         sh;
    
    void write(std::ofstream &f, enum Format format = Binary);
    void write(path_t p, enum Format format = Binary);
    var(nullptr_t p);
    var(Type t = Undefined, Type c = Undefined, size_t count = 0);
    var(Type t, u *n) : t(t), n((u *)n) { } // todo: change this to union type.
    var(path_t p, enum var::Format format);
    
    std::vector<int> shape();
    void set_shape(std::vector<int> v_shape);
    
    var tag(map<str, var> m);
    var(std::ifstream &f, enum var::Format format);
    
    template <typename T>
    var(std::shared_ptr<T> d, std::vector<int> sh) : sh(sh) {
        T *ptr = d.get();
        this->t = Array;
        this->c = data_type(ptr);
        this->d = std::static_pointer_cast<uint8_t>(d);
        assert(c >= i8 && c <= f64);
    }
    
    template <typename T>
    var(std::shared_ptr<T> d, size_t sz) {
        T *ptr = d.get();
        this->t = Array;
        this->c = data_type(ptr);
        this->d = std::static_pointer_cast<uint8_t>(d);
        this->sh = std::vector<int> { int(sz) };
        assert(c >= i8 && c <= f64);
    }
    var(path_t p);
    operator path_t();
    static bool type_check(var &a, var &b);
    var(void*    v);
    var(var  *ref);
    var(VoidRef  r);
    var(size_t  sz);
    var(float    v);
    var(double   v);
    var(int8_t   v);
    var(uint8_t  v);
    var(int16_t  v);
    var(uint16_t v);
    var(int32_t  v);
    var(uint32_t v);
    var(int64_t  v);
    //var(uint64_t v);
    var(FnFilter v);
    var(Fn       v);
    //var(::map<std::string, var> data_map);
    var(std::vector<var> data_array);
    var(std::string str);
    var(const char *str);
    var(path_t p, ::map<std::string, TypeFlags> types = {});
    static ::map<std::string, var> args(int argc, const char* argv[], ::map<std::string, var> def = {});
    
    static int shape_volume(std::vector<int> &sh);
    
    var operator()(var &d) {
        var r;
        if (t == Filter)
            r = ff(d);
        else if (t == Lambda)
            fn(d);
        return r;
    }
    
    template <typename T>
    T value(std::string key, T default_v) {
        if (!m || m->count(key) == 0)
            return default_v;
        return T((*m)[key]);
    }
    
    template <typename K, typename T>
    var(::map<K, T> mm) : t(Map) {
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        for (auto &[k, v]: mm)
            (*m)[k] = var(v);
    }
    
    template <typename T>
    var(std::vector<T> aa) : t(Array) {
        T     *va = aa.data();
        size_t sz = aa.size();
        c = data_type(va);
        a = std::shared_ptr<std::vector<var>>(new std::vector<var>);
        a->reserve(sz);
        if constexpr ((std::is_same_v<T,     bool>)
                   || (std::is_same_v<T,   int8_t>) || (std::is_same_v<T,  uint8_t>)
                   || (std::is_same_v<T,  int16_t>) || (std::is_same_v<T, uint16_t>)
                   || (std::is_same_v<T,  int32_t>) || (std::is_same_v<T, uint32_t>)
                   || (std::is_same_v<T,  int64_t>) || (std::is_same_v<T, uint64_t>)
                   || (std::is_same_v<T,    float>) || (std::is_same_v<T,   double>)) {
            size_t ts = type_size(c);
            sh = { int(sz) };
            d = std::shared_ptr<uint8_t>((uint8_t *)calloc(ts, sz));
            flags = Flags::Compact;
            T *vdata = (T *)d.get();
            memcopy(vdata, va, sz);
            assert(data_type(vdata) == c);
            // can we treat data as a window anymore, with strict use of smart pointers?
            // if data is a reference it should set; and thats what we're doing here...
            for (size_t i = 0; i < sz; i++)
                a->push_back(var {c, (var::u *)&vdata[i]});
          } else {
              for (size_t i = 0; i < sz; i++)
                  a->push_back(aa[i]);
          }
    }
             
    template <typename T>
    static Type data_type(T *v) {
            if constexpr (std::is_same_v<T,       Fn>) return Lambda;
       else if constexpr (std::is_same_v<T, FnFilter>) return Filter;
       else if constexpr (std::is_same_v<T,     bool>) return Bool;
       else if constexpr (std::is_same_v<T,   int8_t>) return   i8;
       else if constexpr (std::is_same_v<T,  uint8_t>) return  ui8;
       else if constexpr (std::is_same_v<T,  int16_t>) return  i16;
       else if constexpr (std::is_same_v<T, uint16_t>) return ui16;
       else if constexpr (std::is_same_v<T,  int32_t>) return  i32;
       else if constexpr (std::is_same_v<T, uint32_t>) return ui32;
       else if constexpr (std::is_same_v<T,  int64_t>) return  i64;
       else if constexpr (std::is_same_v<T, uint64_t>) return ui64;
       else if constexpr (std::is_same_v<T,    float>) return  f32;
       else if constexpr (std::is_same_v<T,   double>) return  f64;
       else return Map;
        assert(false);
    }
    
    template <typename T>
    var(T *v, std::vector<int> sh) : t(Array), c(data_type(v)), sh(sh) {
        int sz = 1;
        for (auto d: sh)
            sz *= d;
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, sz * sizeof(T));
        flags = Compact;
    }
    
    template <typename T>
    var(T *v, size_t sz) : t(Array), c(data_type(v)), sh({ int(sz) }) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, sz * sizeof(T));
        flags = Compact;
    }
    
    // todo: convert Args to a typedef of shared_ptr<map<string, var>>
    inline operator::map<std::string, var> *() {
        return m.get();
    }
    
    inline var &operator[] (const char *s) {
        if (!m)
             m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        return (*m)[std::string(s)];
    }
    
    var &operator[] (str s);
    
    // probably best to switch to value
    inline var &operator[] (const size_t i) {
        int sz = int(shape_volume(sh));
        bool needs_refs = (sz > 0 && !a);
        if (needs_refs) {
            a          = std::shared_ptr<std::vector<var>>(new std::vector<var>());
            uint8_t *v = (uint8_t *)d.get();
            size_t  ts = type_size(c);
            a->reserve(sz);
            for (int i = 0; i < sz; i++, v += ts)
                a->push_back(var {c, (u *)v});
        }
        return (*a)[i];
    }
    
    inline size_t count(std::string v) {
        return m ? m->count(v) : 0;
    }
    
    template <typename T>
    inline operator    std::shared_ptr<T> () { return std::static_pointer_cast<T>(d); }

    inline operator         Fn&() { return fn;               }
    inline operator   FnFilter&() { return ff;               }
    inline operator      int8_t() { return *((  int8_t *)n); }
    inline operator     uint8_t() { return *(( uint8_t *)n); }
    inline operator     int16_t() { return *(( int16_t *)n); }
    inline operator    uint16_t() { return *((uint16_t *)n); }
    inline operator     int32_t() { return *(( int32_t *)n); }
    inline operator    uint32_t() { return *((uint32_t *)n); }
    inline operator     int64_t() { return *(( int64_t *)n); }
    inline operator    uint64_t() { return *((uint64_t *)n); }
    inline operator       void*() { assert(t == Ref); return n; };
    inline operator       node*() { assert(t == Ref); return (node*)n; };
    inline operator       float() { return *((float    *)n); }
    inline operator      double() { return *((double   *)n); }
           operator std::string();
           operator        bool();
    
    bool operator!();
    
    template <typename T>
    T *data() {
        //compact();
        return (T *)d.get();
    }
    
    var(str s);
    
    void reserve(size_t sz);
    size_t size() const;
    void copy(var &ref);
    
   ~var();
    var(const var &ref);
    
    bool operator!=(const var &ref);
    bool operator==(const var &ref);
    bool operator==(var &ref);
    bool operator==(enum Type t) { return this->t == t; }
    
    var& operator=(const var &ref);
    bool operator==(::map<std::string, var> &m) const;
    bool operator!=(::map<std::string, var> &m) const;

    bool operator==(uint16_t v);
    bool operator==( int16_t v);
    bool operator==(uint32_t v);
    bool operator==( int32_t v);
    bool operator==(uint64_t v);
    bool operator==( int64_t v);
    bool operator==(float    v);
    bool operator==(double   v);
    /*
    template <typename T>
    operator std::vector<T> () {
        return *a;
    }*/
    
    operator map<std::string, var> &() {
        return *m;
    }

    void operator += (var d);
    
    static var read_json(str &js, ::map<std::string, TypeFlags> flags = {});

    template <typename T>
    operator T *() {
        return data<T>();
    }
    
    template <typename T>
    var& operator=(std::vector<T> aa) {
        m = null;
        t = Array;
        c = Any;
        int types = 0;
        int last_type = -1;
        a = std::shared_ptr<std::vector<var>>(new std::vector<var>);
        a->reserve(aa.size());
        for (auto &v: aa) {
            var d = var(v);
            if (last_type != d.t) {
                last_type  = d.t;
                types++;
            }
            *a += v;
        }
        if (types == 1)
            c = Type(last_type);
        return *this;
    }
    
    template <typename T>
    var& operator=(::map<std::string, T> mm) {
        a = null;
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        for (auto &[k, v]: mm)
            (*m)[k] = v;
        return *this;
    }
    
    /// rename lambda to Fn
    var &operator=(Fn _fn) {
         t = Lambda;
        fn = _fn;
        return *this;
    }
    
    var &operator=(FnFilter _ff) {
         t = Filter;
        ff = _ff;
        return *this;
    }
    
    void compact();
    static std::shared_ptr<uint8_t> encode(const char *data, size_t len);
    static std::shared_ptr<uint8_t> decode(const char *b64,  size_t b64_len, size_t *alloc);
    void each(FnEach each);
    void each(FnArrayEach each);
};

#include <data/vec.hpp>

class Ref {
protected:
    void *v = null;
public:
    Ref(nullptr_t n = nullptr) : v(n) { }
    Ref(node *n) : v(n) { }
    Ref(void *v) : v(v) { }
    Ref(var &d) : v((void *)d) { }
    operator void *() {
        return v;
    }
};

#define serializer(C,E) \
    C(nullptr_t n) : C()       { }                      \
    C(const C &ref)            { copy(ref);            }\
    C(var &d)                 { import_data(d);       }\
    operator var()            { return export_data(); }\
    operator bool()  const     { return   E;           }\
    bool operator!() const     { return !(E);          }\
    C &operator=(const C &ref) {\
        if (this != &ref)\
            copy(ref);\
        return *this;\
    }\


#include <data/array.hpp>
#include <data/string.hpp>
#include <data/async.hpp>
#include <data/rand.hpp>

typedef map<std::string, var> Args;
void _log(str t, std::vector<var> a, bool error);

struct Logging {
    static void log(str t, std::vector<var> a = {}) { // add this operation to Array
        _log(t, a, false);
    }
    
    static void error(str t, std::vector<var> a = {}) {
        _log(t, a, true);
    }
};

typedef std::function<void(map<std::string, var> &)> FnArgs;

extern Logging console;

typedef std::function<var(var &)> FnProcess;

#if !defined(WIN32) && !defined(WIN64)
#define UNIX
#endif

struct node;
typedef node *                     (*FnFactory)();

struct Element {
    FnFactory       factory = null;
    Args            args;
    vec<Element>    elements;
    node           *ref = 0;
    FnFilter        fn_filter;
    FnFilterMap     fn_filter_map;
    FnFilterArray   fn_filter_arr;
    Element(node *ref) : ref(ref) { }
    Element(nullptr_t n) { }
    Element(var &ref, FnFilterArray f);
    Element(var &ref, FnFilterMap f);
    Element(var &ref, FnFilter f);
    Element(FnFactory factory, Args &args, vec<Element> &elements):
        factory(factory), args(args), elements(elements) { }
    bool operator==(Element &b);
    bool operator!=(Element &b);
    operator bool()  { return ref || factory;     }
    bool operator!() { return !(operator bool()); }
    static Element each(var &d, FnFilter f);
    static Element each(var &d, FnFilterArray f); // a ref can always just be a data pointer, and that data can contain the void ptr or just be a data value
    static Element each(var &d, FnFilterMap f); // var(&data) ? var::ref(data) is clearer.
};

typedef std::function<Element(Args &)> FnRender;


template <typename T>
bool equals(T v, vec<T> values) {
    return values.index_of(v) >= 0;
}

template <typename T>
bool isnt(T v, vec<T> values) {
    return !equals(v, values);
}
