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

struct node;
struct Data;
typedef std::function<void(Data &)>                 Fn;
typedef std::function<Data(Data &)>                 FnFilter;
typedef std::function<Data(Data &, std::string &)>  FnFilterMap;
typedef std::function<Data(Data &, size_t)>         FnFilterArray;
typedef std::function<void(Data &, std::string &)>  FnEach;
typedef std::function<void(Data &, size_t)>         FnArrayEach;

struct VoidRef {
    void *v;
    VoidRef(void *v) : v(v) { }
};

struct str;

struct Data {
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
        Data::Type t;
        int flags;
    };
    
    static size_t type_size(Data::Type t);
    
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
    std::shared_ptr<::map<std::string, Data>> m = nullptr;
    std::shared_ptr<std::vector<Data>>        a = nullptr;
    std::shared_ptr<std::string>              s = nullptr;
    u                                        *n = nullptr; // we CAN utilize n.
    
    size_t               d_size = 0;
    int                   flags = 0;
    std::vector<int>     *shape = NULL;
    
    Data(nullptr_t p);
    Data(Type t = Undefined, Type c = Undefined, size_t count = 0);
    Data(Type t, int64_t *n) : t(t), n((u *)n) { }
    Data(std::filesystem::path p);
    operator std::filesystem::path();
    Data(void*    v);
    Data(Data  *ref);
    Data(VoidRef  r);
    Data(size_t  sz);
    Data(float    v);
    Data(double   v);
    Data(int8_t   v);
    Data(uint8_t  v);
    Data(int16_t  v);
    Data(uint16_t v);
    Data(int32_t  v);
    Data(uint32_t v);
    Data(int64_t  v);
    //Data(uint64_t v);
    Data(FnFilter v);
    Data(Fn       v);
    //Data(::map<std::string, Data> data_map);
    Data(std::vector<Data> data_array);
    Data(std::string str);
    Data(const char *str);
    Data(std::filesystem::path p, ::map<std::string, TypeFlags> types = {});
    static ::map<std::string, Data> args(int argc, const char **argv);
    
    Data operator()(Data &d) {
        Data r;
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
    Data(::map<K, T> mm) : t(Map) {
        m = std::shared_ptr<::map<std::string, Data>>(new ::map<std::string, Data>);
        for (auto &[k, v]: mm)
            (*m)[k] = Data(v);
    }
    
    template <typename T>
    Data(std::vector<T> aa) : t(Array) {
        T     *va = aa.data();
        size_t sz = aa.size();
        c = data_type(va);
        a = std::shared_ptr<std::vector<Data>>(new std::vector<Data>);
        a->reserve(sz);
        if constexpr ((std::is_same_v<T,     bool>)
                   || (std::is_same_v<T,   int8_t>) || (std::is_same_v<T,  uint8_t>)
                   || (std::is_same_v<T,  int16_t>) || (std::is_same_v<T, uint16_t>)
                   || (std::is_same_v<T,  int32_t>) || (std::is_same_v<T, uint32_t>)
                   || (std::is_same_v<T,  int64_t>) || (std::is_same_v<T, uint64_t>)
                   || (std::is_same_v<T,    float>) || (std::is_same_v<T,   double>)) {
            size_t ts = type_size(c);
            d_size = sz;
            d = std::shared_ptr<uint8_t>((uint8_t *)malloc(ts * sz));
            flags = Flags::Compact;
            T *vdata = (T *)d.get();
            memcopy(vdata, va, sz);
            assert(data_type(vdata) == c);
            for (size_t i = 0; i < sz; i++)
                a->push_back(Data {c, (int64_t *)(&vdata[i])});
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
       else if constexpr (std::is_same_v<T,   int8_t>) return i8;
       else if constexpr (std::is_same_v<T,  uint8_t>) return ui8;
       else if constexpr (std::is_same_v<T,  int16_t>) return i16;
       else if constexpr (std::is_same_v<T, uint16_t>) return ui16;
       else if constexpr (std::is_same_v<T,  int32_t>) return i32;
       else if constexpr (std::is_same_v<T, uint32_t>) return ui32;
       else if constexpr (std::is_same_v<T,  int64_t>) return i64;
       else if constexpr (std::is_same_v<T, uint64_t>) return ui64;
       else if constexpr (std::is_same_v<T,    float>) return f32;
       else if constexpr (std::is_same_v<T,   double>) return f64;
       else return Map;
        assert(false);
    }
    
    template <typename T>
    Data(T *v, size_t sz) : t(Array), c(data_type(v)), d_size(sz) {
        d = std::shared_ptr<T>(new T[sz]);
        memcopy(d.get(), v, sz);
        flags = Compact;
    }
    
    // todo: convert Args to a typedef of shared_ptr<map<string, Data>>
    inline operator::map<std::string, Data> *() {
        return m.get();
    }
    
    inline Data &operator[] (const char *s) { // one Data per dimension, perfecto.
        return (*m)[std::string(s)];
    }
    
    Data &operator[] (str s);
    
    inline Data &operator[] (const size_t i) {
        compact();
        return (*a)[i];
    }
    
    inline size_t count(std::string v) {
        return m ? m->count(v) : 0;
    }
    
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
        compact();
        return (T *)d.get();
    }
    
    Data(str s);
    
    void reserve(size_t sz);
    size_t size() const;
    void copy(Data &ref);
    
   ~Data();
    Data(const Data &ref);
    
    bool operator!=(const Data &ref);
    bool operator==(const Data &ref);
    bool operator==(Data &ref);
    bool operator==(enum Type t) { return this->t == t; }
    
    Data& operator=(const Data &ref);
    bool operator==(::map<std::string, Data> &m) const;
    bool operator!=(::map<std::string, Data> &m) const;

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
    
    operator map<std::string, Data> &() {
        return *m;
    }

    void operator += (Data d);
    
    void read_json(str &js, ::map<std::string, TypeFlags> flags = {});

    template <typename T>
    Data& operator=(std::vector<T> aa) {
        m = null;
        t = Array;
        c = Any;
        int types = 0;
        int last_type = -1;
        a = std::shared_ptr<std::vector<Data>>(new std::vector<Data>);
        a->reserve(aa.size());
        for (auto &v: aa) {
            Data d = Data(v);
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
    Data& operator=(::map<std::string, T> mm) {
        a = null;
        m = std::shared_ptr<::map<std::string, Data>>(new ::map<std::string, Data>);
        for (auto &[k, v]: mm)
            (*m)[k] = v;
        return *this;
    }
    
    /// rename lambda to Fn
    Data &operator=(Fn _fn) {
         t = Lambda;
        fn = _fn;
        return *this;
    }
    
    Data &operator=(FnFilter _ff) {
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
    Ref(Data &d) : v((void *)d) { }
    operator void *() {
        return v;
    }
};

/// Standard Data.  You must have data as a special friend.
#define serializer(C,E) \
    C(nullptr_t n) : C()       { }                      \
    C(const C &ref)            { copy(ref);            }\
    C(Data &d)                 { import_data(d);       }\
    operator Data()            { return export_data(); }\
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

typedef map<std::string, Data> Args;
void _log(str t, std::vector<Data> a, bool error);

struct Logging {
    static void log(str t, std::vector<Data> a = {}) { // add this operation to Array
        _log(t, a, false);
    }
    
    static void error(str t, std::vector<Data> a = {}) {
        _log(t, a, true);
    }
};

typedef std::function<void(map<std::string, Data> &)> FnArgs;

extern Logging console;

typedef std::function<Data(Data &)> FnProcess;

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
    Element(Data &ref, FnFilterArray f);
    Element(Data &ref, FnFilterMap f);
    Element(Data &ref, FnFilter f);
    Element(FnFactory factory, Args &args, vec<Element> &elements):
        factory(factory), args(args), elements(elements) { }
    bool operator==(Element &b);
    bool operator!=(Element &b);
    operator bool()  { return ref || factory;     }
    bool operator!() { return !(operator bool()); }
    static Element each(Data &d, FnFilter f);
    static Element each(Data &d, FnFilterArray f); // a ref can always just be a data pointer, and that data can contain the void ptr or just be a data value
    static Element each(Data &d, FnFilterMap f); // Data(&data) ? Data::ref(data) is clearer.
};

typedef std::function<Element(Args &)> FnRender;
