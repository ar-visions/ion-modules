#pragma once
#define _USE_MATH_DEFINES
#include <iostream>
#include <unordered_map>
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
#include <type_traits>
#include <dx/type.hpp>

static inline
int64_t ticks() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

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

typedef std::filesystem::path           path_t;
typedef std::filesystem::file_time_type path_date_t;

void resources(path_t p, std::vector<const char *> exts, std::function<void(path_t &)> fn);
bool exists(path_t &p);

#include <dx/map.hpp>

struct node;
struct var;
typedef std::function<bool()>                      FnBool;
typedef std::function<void(void *)>                FnArb;
typedef std::function<void(void)>                  FnVoid;
typedef std::function<void(var &)>                 Fn;
typedef std::function<void(var &, node *)>         FnNode;
typedef std::function<var()>                       FnGet;
//typedef std::function< var(var &)>                 FnFilter;
//typedef std::function< var(var &, std::string &)>  FnFilterMap;
//typedef std::function< var(var &, size_t)>         FnFilterArray;
typedef std::function<void(var &, std::string &)>  FnEach;
typedef std::function<void(var &, size_t)>         FnArrayEach;

struct VoidRef {
    void *v;
    VoidRef(void *v) : v(v) { }
};

struct str;

struct var {
    Type::Specifier t = Type::Undefined;
    Type::Specifier c = Type::Undefined;

    enum Binding {
        Insert,
        Update,
        Delete
    };

    enum Flags {
        Encode   = 1,
        Compact  = 2,
        ReadOnly = 4,
        DestructAttached = 8
    };
    
    struct TypeFlags {
        Type::Specifier t;
        int flags;
    };
    
    typedef ::map<std::string, TypeFlags> FieldFlags;
    
    enum Format {
        Binary,
        Json
    };
    
    union u {
        bool    vb;
        int8_t  vi8;
        int16_t vi16;
        int32_t vi32;
        int64_t vi64;
        float   vf32;
        double  vf64;
        var    *vref;
        node   *vnode;
        void   *vstar;
    };
    u                   n_value = { 0 };
    Fn                       fn;
  //FnFilter                 ff;
    std::shared_ptr<uint8_t>                  d = nullptr;
    std::shared_ptr<::map<std::string, var>>  m = nullptr;
    std::shared_ptr<std::vector<var>>         a = nullptr;
    std::shared_ptr<std::string>              s = nullptr;
    u                                        *n = nullptr; // we CAN utilize n.
    
    int                   flags = 0;
    std::vector<int>         sh;
    
//protected:
    ///
    /// observer fn and pointer (used by Storage controllers -- todo: merge into binding allocation via class
    std::function<void(Binding op, var &row, str &field, var &value)> fn_change;
    std::string  field    = "";
    var         *row      = null;
    var         *observer = null;

    void observe_row(var &row);
    
    static var &resolve(var &in);
    
    static char *parse_obj(  var &scope, char *start, std::string field, FieldFlags &flags);
    static char *parse_arr(  var &scope, char *start, std::string field, FieldFlags &flags);
    static char *parse_value(var &scope, char *start, std::string field, Type::Specifier enforce_type, FieldFlags &flags);
    
public:
    
    var *ptr() { return this; }
    void clear();
    bool operator==(Type::Specifier tt);
    void observe(std::function<void(Binding op, var &row, str &field, var &value)> fn);
    void write(std::ofstream &f, enum Format format = Binary);
    void write(path_t p, enum Format format = Binary);
    var(nullptr_t p);
    var(Type::Specifier t, void *v) : t(t) {
        assert(t == Type::Arb);
        n_value.vstar = (void *)v;
    }
    var(Type::Specifier t = Type::Undefined, Type::Specifier c = Type::Undefined, size_t count = 0);
    var(Type::Specifier t, u *n) : t(t), n((u *)n) { }
    var(Type::Specifier t, std::string str);
    var(var *vref) : t(Type::Ref), n(null) {
        assert(vref);
        while (vref->t == Type::Ref)
            vref = vref->n_value.vref; // refactor the names.
        n_value.vref = vref;
    }
    var(path_t p, Format format);
    
    std::vector<int> shape();
    void set_shape(std::vector<int> v_shape);
    void attach(std::string name, void *arb, std::function<void(var &)> fn) {
        map()[name]        = var { Type::Arb, arb };
        map()[name].flags |= DestructAttached;
        map()[name].fn     = fn;
    }
    size_t attachments() {
        size_t r = 0;
        for (auto &[k,v]:map())
            if (v.flags & DestructAttached)
                r++;
        return r;
    }
    var tag(::map<std::string, var> m);
    var(std::ifstream &f, enum var::Format format);
    //var(str templ, vec<var> arr);
    var(Type::Specifier c, std::vector<int> sh) : sh(sh) {
        size_t sz   = shape_volume(sh);
        size_t t_sz = Type::size(c);
        this->t = Type::Array;
        this->c = c; //data_type(ptr);
        this->d = std::shared_ptr<uint8_t>((uint8_t *)calloc(t_sz, sz));
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, std::vector<int> sh) : sh(sh) {
        this->t = Type::Array;
        this->c = c; //data_type(ptr);
        this->d = std::static_pointer_cast<uint8_t>(d);
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, size_t sz) {
        this->t = Type::Array;
        this->c = c; //data_type(ptr);
        this->d = std::static_pointer_cast<uint8_t>(d);
        this->sh = std::vector<int> { int(sz) };
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    var(path_t p);
    operator path_t();
    static bool type_check(var &a, var &b);
    var(void*    v);
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
    var(int64_t  v, int flags = 0);
  //var(uint64_t v);
  //var(FnFilter v);
    var(Fn       v);
  //var(::map<std::string, var> data_map);
    var(std::vector<var> data_array);
    var(std::string str);
    var(const char *str);
    static var load(path_t p);
    static ::map<std::string, var> args(
        int argc, const char* argv[], ::map<std::string, var> def = {}, std::string field_field = "");
    
    static int shape_volume(std::vector<int> &sh);
    
    ::map<std::string, var> &map() { /// refs cant have their own map
        var &v   = var::resolve(*this);
        if (!v.m)
             v.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        return *v.m;
    }
    
    var operator()(var &d) {
        var &v = var::resolve(*this);
        var res;
        if (v == Type::Lambda)
            fn(d);
        return res;
    }
    
    template <typename T>
    T convert() {
        var &v = var::resolve(*this);
        assert(v == Type::ui8);
        assert(d != null);
        assert(size() == sizeof(T)); /// C3PO: that isn't very reassuring.
        return T(*(T *)d.get());
    }
    
    template <typename T>
    T value(std::string key, T default_v) {
        var &v = var::resolve(*this);
        if (!v.m || v.m->count(key) == 0)
            return default_v;
        return T((*v.m)[key]);
    }
    
    template <typename K, typename T>
    var(::map<K, T> mm) : t(Type::Map) {
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        for (auto &[k, v]: mm)
            (*m)[k] = var(v);
    }
    
    template <typename T>
    var(std::vector<T> aa) : t(Type::Array) {
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
            size_t ts = Type::size(c);
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
             
    /// needs to go in Type
    template <typename T>
    static Type::Specifier data_type(T *v) {
             if constexpr (std::is_same_v<T,       Fn>) return Type::Lambda;
      //else if constexpr (std::is_same_v<T, FnFilter>) return Type::Filter; -- global 'filter' notion not strong enough, and requires high amounts of internal conversion
        else if constexpr (std::is_same_v<T,     bool>) return Type::Bool;
        else if constexpr (std::is_same_v<T,   int8_t>) return Type::i8;
        else if constexpr (std::is_same_v<T,  uint8_t>) return Type::ui8;
        else if constexpr (std::is_same_v<T,  int16_t>) return Type::i16;
        else if constexpr (std::is_same_v<T, uint16_t>) return Type::ui16;
        else if constexpr (std::is_same_v<T,  int32_t>) return Type::i32;
        else if constexpr (std::is_same_v<T, uint32_t>) return Type::ui32;
        else if constexpr (std::is_same_v<T,  int64_t>) return Type::i64;
        else if constexpr (std::is_same_v<T, uint64_t>) return Type::ui64;
        else if constexpr (std::is_same_v<T,    float>) return Type::f32;
        else if constexpr (std::is_same_v<T,   double>) return Type::f64;
        else return Type::Map;
        assert(false);
    }
    
    template <typename T>
    var(T *v, std::vector<int> sh) : t(Type::Array), c(data_type(v)), sh(sh) {
        int sz = 1;
        for (auto d: sh)
            sz *= d;
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, sz * sizeof(T)); // in bytes
        flags = Compact;
    }
    
    template <typename T>
    var(T *v, size_t sz) : t(Type::Array), c(data_type(v)), sh({ int(sz) }) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, sz * sizeof(T)); // in bytes
        flags = Compact;
    }
    
    // todo: convert Args to a typedef of shared_ptr<map<string, var>>
    inline operator::map<std::string, var> *() {
        return m.get();
    }
    
    inline var &operator[] (const char *s) {
        var &v = var::resolve(*this);
        if (!v.m)
             v.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        return (*v.m)[std::string(s)];
    }
    
    var &operator[] (str s);
    
    inline var &operator[] (const size_t i) {
        var &v = var::resolve(*this);
        int sz = int(shape_volume(v.sh));
        bool needs_refs = (sz > 0 && !v.a); ///#
        if (needs_refs) {
            v.a         = std::shared_ptr<std::vector<var>>(new std::vector<var>());
            uint8_t *u8 = (uint8_t *)v.d.get();
            size_t   ts = Type::size(v.c);
            v.a->reserve(sz);
            for (int i = 0; i < sz; i++)
                v.a->push_back(var {v.c, (u *)&u8[i * ts]});
        }
        return (*v.a)[i];
    }
    
    inline size_t count(std::string fv) {
        var &v = var::resolve(*this);
        if (v.t == Type::Array) {
            if (a) {
                var conv  = fv;
                int count = 0;
                for (auto &v: *a)
                    if (conv == v)
                        count++;
                return count;
            }
        }
        if (v.m) /// no creation of this map unless required (change cases)
            return v.m->count(fv);
        return v.m ? v.m->count(fv) : 0;
    }
    
    template <typename T>
    inline operator    std::shared_ptr<T> () { return std::static_pointer_cast<T>(d); }
    
    /// no implicit type conversion [for the most part], easier to spot bugs this way
    /// strict/non-strict modes should be appropriate in the future but its probably too much now
    inline operator         Fn&() { return fn;               }
  //inline operator   FnFilter&() { return ff;               }
    inline operator      int8_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return int8_t(*v.n_value.vref);
        return *((  int8_t *)v.n);
    }
    inline operator     uint8_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint8_t(*v.n_value.vref);
        return *(( uint8_t *)v.n);
    }
    inline operator     int16_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return *(( int16_t *)v.n);
    }
    inline operator    uint16_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return *((uint16_t *)v.n);
    }
    inline operator     int32_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return int32_t(*v.n_value.vref);
        if (v.t == Type::Str)
            return atoi(v.s.get()->c_str());
        return *(( int32_t *)v.n);
    }
    inline operator    uint32_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint32_t(*v.n_value.vref);
        return *((uint32_t *)v.n);
    }
    inline operator     int64_t() {
        if (t == Type::Ref)
            return int64_t(*n_value.vref);
        return *(( int64_t *)n);
    }
    inline operator    uint64_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint64_t(*v.n_value.vref);
        return *((uint64_t *)v.n);
    }
    inline operator       void*() { /// choppy cross usage here, and we need to fix up node refs
        var &v = var::resolve(*this);
        assert(v.t == Type::Ref);           /// best thing to do is make node* a union member
        return v.n;
    };
    inline operator       node*() {   /// we can get rid of this n usage in favor of v.vnode [todo]
        var &v = var::resolve(*this);
        assert(v.t == Type::Ref);
        return (node*)v.n;
    };
    inline operator       float() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref) return float(*v.n_value.vref);
        assert( v.t == Type::f32 || v.t == Type::f64);
        return (v.t == Type::f32) ? *(float *)v.n :
               (v.t == Type::f64) ?   float(*(double *)v.n) :
               (v.t == Type::Str) ?   float(std::stod(*v.s)) : 0.0;
    }
    inline operator      double() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref) return double(*v.n_value.vref);
        return (v.t == Type::f64) ? *(double *)v.n :
               (v.t == Type::f32) ?   double(*(float *)v.n) :
               (v.t == Type::Str) ? std::stod(*v.s) : 0.0;
    }
           operator std::string();
           operator bool();
    
    bool operator!();
    
    template <typename T>
    T *data() {
        var &v = var::resolve(*this);
        //v.compact();
        return (T *)v.d.get();
    }
    
    var(str s);
    
    void reserve(size_t sz);
    size_t size();
    void copy(var &ref);
    var copy();
    
   ~var();
    var(const var &ref);
    
    bool operator!=(const var &ref);
    bool operator==(const var &ref);
    bool operator==(var &ref);
    
    var& operator=(const var &ref);
    bool operator==(::map<std::string, var> &m);
    bool operator!=(::map<std::string, var> &m);

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
    
    operator ::map<std::string, var> &() {
        return *m;
    }

    void operator += (var d);
    
    static var parse_json(str &js);

    template <typename T>
    operator T *() {
        return data<T>();
    }
    void notify_insert();
    void notify_delete();
    void notify_update();
    
    template <typename T>
    var& operator=(std::vector<T> aa) {
        m = null;
        t = Type::Array;
        c = Type::Any;
        Type::Specifier last_type = Type::Specifier(-1);
        int types = 0;
        a = std::shared_ptr<std::vector<var>>(new std::vector<var>);
        a->reserve(aa.size());
        for (auto &v: aa) {
            var d = var(v);
            if (d.t != last_type) {
                last_type  = Type::Specifier(size_t(d.t)); // dont look at me
                types++;
            }
            *a += v;
        }
        if (types == 1)
            c = last_type;
        
        notify_update();
        return *this;
    }
    
    template <typename T>
    var& operator=(::map<std::string, T> mm) {
        a = null;
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
        for (auto &[k, v]: mm)
            (*m)[k] = v;
        notify_update();
        return *this;
    }
    
    /// rename lambda to Fn
    var &operator=(Fn _fn) {
         t = Type::Lambda;
        fn = _fn;
        notify_update();
        return *this;
    }
    
    /*
    var &operator=(FnFilter _ff) {
         t = Filter;
        ff = _ff;
        notify_update();
        return *this;
    }
    */
    
    void compact();
    static std::shared_ptr<uint8_t> encode(const char *data, size_t len);
    static std::shared_ptr<uint8_t> decode(const char *b64,  size_t b64_len, size_t *alloc);
    
    static bool is_numeric(const char *s);
    static std::string parse_numeric(char **cursor);
    static std::string parse_quoted(char **cursor, size_t max_len = 0);
    
    void each(FnEach each);
    void each(FnArrayEach each);
    
    var select_first(std::function<var(var &)> fn);
    std::vector<var> select(std::function<var(var &)> fn);
};

#include <dx/vec.hpp>

/// beat this with a stick later
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

#define io_shim(C,E) \
    C(nullptr_t n) : C()       { }                      \
    C(const C &ref)            { copy(ref);            }\
    C(var &d)                  { importer(d);          }\
    operator var()             { return exporter();    }\
    operator bool()  const     { return   E;           }\
    bool operator!() const     { return !(E);          }\
    C &operator=(const C &ref) {\
        if (this != &ref)\
            copy(ref);\
        return *this;\
    }\


#include <dx/array.hpp>
#include <dx/string.hpp>
#include <dx/async.hpp>
#include <dx/rand.hpp>

typedef map<std::string, var> Args;
void _log(str t, std::vector<var> a, bool error);


struct Logging {
#if !defined(NDEBUG)
    static void log(str t, std::vector<var> a = {}) { // add this operation to Array
        _log(t, a, false);
    }
#else
    static inline void log(str t, std::vector<var> a = {}) { }
#endif
    static inline void error(str t, std::vector<var> a = {}) {
        _log(t, a, true);
    }
    static inline void assertion(bool a0, str t, std::vector<var> a = {}) {
        if (!a0) {
            _log(t, a, true);
            assert(a0);
        }
    }
    static inline void fault(str t, std::vector<var> a = {}) {
        _log(t, a, true);
        exit(1);
    }
    template <typename T>
    static T prompt(str t, std::vector<var> a = {}) {
        _log(t, a, false);
        T v;
        std::cin >> v;
        return v;
    }
};

typedef std::function<void(map<std::string, var> &)> FnArgs;

extern Logging console;

typedef std::function<var(var &)> FnProcess;

#if !defined(WIN32) && !defined(WIN64)
#define UNIX
#endif

struct Bind {
    str id, to;
    Bind(str id, str to = null):id(id), to(to ? to : id) { }
    bool operator==(Bind &b) { return id == b.id; }
    bool operator!=(Bind &b) { return !operator==(b); }
};

struct  node;
typedef node * (*FnFactory)();
typedef vec<Bind> Binds;

struct Element {
    FnFactory       factory = null;
    Binds           binds;
    vec<Element>    elements;
    node           *ref = 0;
    std::string     idc = "";
    ///
    std::string &id() {
        if (idc.length())
            return idc;
        str id;
        for (auto &bind: binds)
            if (bind.id == "id") {
                id       = bind.to;
                break;
            }
        char buf[256];
        if (!id)
            sprintf(buf, "%p", (void *)factory); /// type-based token, effectively
        idc = id ? std::string(id) : std::string(buf);
        return idc;
    }
    Element(node *ref) : ref(ref) { }
    Element(nullptr_t n = null) { }
    Element(FnFactory factory, Binds &binds, vec<Element> &elements):
        factory(factory), binds(binds), elements(elements) { }
    bool operator==(Element &b);
    bool operator!=(Element &b);
    operator bool()  { return ref || factory;     }
    bool operator!() { return !(operator bool()); }
    
    template <typename T>
    static Element each(vec<T> &i, std::function<Element(T &v)> fn);
    
    template <typename K, typename V>
    static Element each(map<K, V> &m, std::function<Element(K &k, V &v)> fn);
};

typedef std::function<Element(void)> FnRender;

template <typename T>
bool equals(T v, vec<T> values) {
    return values.index_of(v) >= 0;
}

template <typename T>
bool isnt(T v, vec<T> values) {
    return !equals(v, values);
}

/// adding these declarations here.  dont mind me.
enum KeyState {
    KeyUp,
    KeyDown
};

struct KeyStates {
    bool shift;
    bool ctrl;
    bool meta;
};

struct KeyInput {
    int key;
    int scancode;
    int action;
    int mods;
};

enum MouseButton {
    LeftButton,
    RightButton,
    MiddleButton
};

enum KeyCode {
    D           = 68,
    N           = 78,
    Backspace   = 8,
    Tab         = 9,
    LineFeed    = 10,
    Return      = 13,
    Shift       = 16,
    Ctrl        = 17,
    Alt         = 18,
    Pause       = 19,
    CapsLock    = 20,
    Esc         = 27,
    Space       = 32,
    PageUp      = 33,
    PageDown    = 34,
    End         = 35,
    Home        = 36,
    Left        = 37,
    Up          = 38,
    Right       = 39,
    Down        = 40,
    Insert      = 45,
    Delete      = 46, // or 127 ?
    Meta        = 91
};

typedef map<str, var> Schema;
typedef map<str, var> Map;
typedef vec<var>      Table;
typedef map<str, var> ModelMap;

inline var ModelDef(str name, Schema schema) {
    var m   = schema;
        m.s = std::shared_ptr<std::string>(new std::string(name));
    return m;
}

struct Remote {
        int64_t value;
            str remote;
    Remote(int64_t value)                  : value(value)                 { }
    Remote(nullptr_t n = nullptr)          : value(-1)                    { }
    Remote(str remote, int64_t value = -1) : value(value), remote(remote) { }
    operator int64_t() { return value; }
    operator var() {
        if (remote) {
            var m = { Type::Map, Type::Undefined }; /// invalid state used as trigger.
                m.s = std::shared_ptr<std::string>(new std::string(remote));
            m["resolves"] = remote;
            return m;
        }
        return var(value);
    }
};
///
struct Ident   : Remote {
       Ident() : Remote(null) { }
};
///
struct Symbol {
    int         value;
    std::string symbol;
    bool operator==(int v) { return v == value; }
};
///
typedef vec<Symbol> Symbols;
    
/// basic enums type needs to be enumerable, serializable, and introspectable
struct EnumData:io {
    Type        type;
    int         kind;
    EnumData(Type type, int kind): type(type), kind(kind) { }
};

template <typename T>
struct ex:EnumData {
    ex(int kind) : EnumData(Id<T>(), kind) { }
    operator bool() { return kind > 0; }
    operator  var() {
        for (auto &sym: T::symbols)
            if (sym.value == kind)
                return sym.symbol;
        console.assertion(false, "invalid enum");
        return null;
    }
    /// depending how Symbol * param, this is an assertion failure on a resolve of symbol to int
    int resolve(std::string str, Symbol **ptr = null) {
        for (auto &sym: T::symbols)
            if (sym.symbol == str) {
                *ptr = &sym;
                return sym.value;
            }
        if (ptr)
            *ptr = null;
        else
            console.error("enum resolve assertion fail: {0}", {str});
        ///
        console.assertion(T::symbols.size(), "empty symbols");
        return T::symbols[0].value; /// could be null, or undefined-like functionality
    }
    ///
    ex(var &v) { kind = resolve(v); }
};
