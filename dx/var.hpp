/// convert to seed-based var design, just an array or queue of data structures, shared structures.
///
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
#include <dx/array.hpp>
#include <dx/map.hpp>
#include <dx/string.hpp>

static inline
int64_t ticks() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

template <class T>
static inline T interp(T from, T to, T f) { return from * (T(1) - f) + to * f; }

template <typename T>
static inline T *allocate(size_t count, size_t align = 16) {
    size_t sz = count * sizeof(T); return (T *)aligned_alloc(align, (sz + (align - 1)) & ~(align - 1));
}

template <typename T>
static inline void memclear(T *dst, size_t count = 1, int value = 0) { memset(dst, value, count * sizeof(T)); }

template <class T>
static inline void memclear(T &dst, size_t count = 1, int value = 0) { memset(&dst, value, count * sizeof(T)); }

template <typename T>
static inline void memcopy(T *dst, T *src, size_t count = 1)         { memcpy(dst, src, count * sizeof(T)); }

struct VoidRef {
    void *v;
    VoidRef(void *v) : v(v) { }
};

struct var:io {
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
    
    typedef map<string, TypeFlags> FieldFlags;
    
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
    
    u                            n_value = { 0 }; /// should be n space (n points to unions elsewhere too such as in-stride shorts, very out of fashion)
    Fn                           fn;
  //FnFilter                     ff;
    std::shared_ptr<uint8_t>     d = null;
    map<string, var>             m = null;
    ::array<var>                 a = null;
    str                          s = null;
    u                           *n = null;
    ///
    int                          flags = 0; // attempt to deprecate
    Shape<Major>                 sh;
//protected:
    /// observer fn and pointer (used by Storage controllers -- todo: merge into binding allocation via class
    std::function<void(Binding op, var &row, str &field, var &value)> fn_change;
    string     field = "";
    var         *row = null;
    var    *observer = null;
    ///
    void observe_row(var &row);
    static var &resolve(var &in);
    static char *parse_obj(  var &scope, char *start, string field, FieldFlags &flags);
    static char *parse_arr(  var &scope, char *start, string field, FieldFlags &flags);
    static char *parse_value(var &scope, char *start, string field, Type::Specifier enforce_type, FieldFlags &flags);
    ///
public:
    ///
    var *ptr() { return this; }
    void clear();
    bool operator==(Type::Specifier tt);
    void observe(std::function<void(Binding op, var &row, str &field, var &value)> fn);
    void write(std::ofstream &f, enum Format format = Binary);
    void write(path_t p, enum Format format = Binary);
    void write_binary(path_t p);
    ///
    var(std::nullptr_t p);
    var(Type::Specifier t, void *v) : t(t), sh(0) {
        assert(t == Type::Arb);
        n_value.vstar = (void *)v;
    }
    var(Type::Specifier t = Type::Undefined, Type::Specifier c = Type::Undefined, size_t count = 0);
    var(Type::Specifier t, u *n) : t(t), n((u *)n) {
        /// unsafe pointer use-case.
        // assert(false);
    }
    var(Type::Specifier t, string str);
    var(Size   sz) : t(Type::i64) { n_value.vi64 = ssize_t(sz); n = &n_value; } // clean this area up. lol [gets mop; todo]
    var(var *vref) : t(Type::Ref), n(null) {
        assert(vref);
        while (vref->t == Type::Ref)
            vref = vref->n_value.vref; // refactor the names.
        n_value.vref = vref;
    }
    var(path_t p, Format format);
    
    Shape<Major> shape();
    void set_shape(Shape<Major> &v_shape);
    void attach(string name, void *arb, std::function<void(var &)> fn) {
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

    var tag(map<string, var> m);
    var(std::ifstream &f, enum var::Format format);
    
    var(Type::Specifier c, Shape<Major> sh) : sh(sh) {
        size_t t_sz = Type::size(c);
        this->t = Type::Array;
        this->c = c;
        this->d = std::shared_ptr<uint8_t>((uint8_t *)calloc(t_sz, size_t(sh)));
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, Shape<Major> sh) : sh(sh) {
        this->t = Type::Array;
        this->c = c;
        this->d = std::static_pointer_cast<uint8_t>(d);
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, Size sz) {
        this->t  = Type::Array;
        this->c  = c;
        this->d  = std::static_pointer_cast<uint8_t>(d);
        this->sh = Shape<Major>(sz); /// Major.
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    var(Type::Specifier c, std::shared_ptr<uint8_t> d, Shape<Major> sh);
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
  //var(map<string, var> data_map);
    var(::array<var> data_array);
    var(string str);
    var(const char *str);
    
    static var load(path_t p);
    
    template <typename T>
    operator ::array<T>() {
        var &v = var::resolve(*this);
        assert(t == Type::Array);
        ::array<T> result(v.size());
        for (auto &i:v.a)
            result += T(i);
        return result;
    }
    
    map<string, var> &map()   { return var::resolve(*this).m; }
    ::array<var>     &array() { return var::resolve(*this).a; }
    var   operator()(var &d) {
        var &v = var::resolve(*this);
        var res;
        if (v == Type::Lambda)
            fn(d);
        return res;
    }
    
    template <typename T>
    T convert() {
        //var &v = var::resolve(*this);
        //assert(v == Type::ui8);
        assert(d != null);
        assert(size() == sizeof(T)); /// C3PO: that isn't very reassuring.
        return T(*(T *)d.get());
    }
    
    template <typename T>
    T value(string key, T default_v) {
        var &v = var::resolve(*this);
        return (v.m.count(key) == 0) ? default_v : T(v.m[key]);
    }
    
    template <typename K, typename T>
    var(::map<K, T> mm) : t(Type::Map) {
        for (auto &[k, v]: mm)
            m[k] = var(v);
    }
    
    template <typename T>
    var(::array<T> aa) : t(Type::Array) {
        T     *va = aa.data();
        Size   sz = aa.size();
        c         = Type::specifier(va);
        a.reserve(sz);
        ///
        if constexpr ((std::is_same_v<T,     bool>)
                   || (std::is_same_v<T,   int8_t>) || (std::is_same_v<T,  uint8_t>)
                   || (std::is_same_v<T,  int16_t>) || (std::is_same_v<T, uint16_t>)
                   || (std::is_same_v<T,  int32_t>) || (std::is_same_v<T, uint32_t>)
                   || (std::is_same_v<T,  int64_t>) || (std::is_same_v<T, uint64_t>)
                   || (std::is_same_v<T,    float>) || (std::is_same_v<T,   double>)) {
            size_t ts = Type::size(c);
            sh        = Shape<Major>(sz);
            d         = std::shared_ptr<uint8_t>((uint8_t *)calloc(ts, sz));
            flags     = Flags::Compact;
            T *vdata  = (T *)d.get();
            memcopy(vdata, va, sz);
            assert(Type::specifier(vdata) == c);
            for (size_t i = 0; i < sz; i++)
                a += var {c, (var::u *)&vdata[i]};
          } else
              for (size_t i = 0; i < sz; i++)
                  a += aa[i];
    }
    
    template <typename T>
    var(T *v, ::array<int> sh) : t(Type::Array), c(Type::specifier(v)), sh(sh) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[size_t(sh)]);
        memcopy(d.get(), (uint8_t *)v, size_t(sh) * sizeof(T)); // in bytes
        flags = Compact;
    }
    
    template <typename T>
    var(T *v, Size sz) : t(Type::Array), c(Type::specifier(v)), sh(sz) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, size_t(sh) * sizeof(T)); // in bytes
        flags = Compact;
    }

         operator ::map<string, var> &() { return m; }
    var &operator[] (const char *s)     { return var::resolve(*this).m[string(s)]; }
    var &operator[] (str s);
    var &operator[] (const size_t i)    {
        var &v = var::resolve(*this);
        int sz = int(v.sh);
        bool needs_refs = (sz > 0 && !v.a);
        if (needs_refs) {
            uint8_t *u8 = (uint8_t *)v.d.get();
            size_t   ts = Type::size(v.c);
            // clear with reserve
            v.a.clear(sz); 
            for (int i = 0; i < sz; i++)
                v.a += var { v.c, (u *)&u8[i * ts] };
        }
        return v.a[i];
    }
    
    size_t count(string fv) {
        var &v = var::resolve(*this);
        if (v.t == Type::Array) {
            if (a) {
                var  conv = fv; // i want this.
                int count = 0;
                for (auto &v: a)
                    if (conv == v)
                        count++;
                return count;
            }
        }
        return v.m.count(fv);
    }
    
    template <typename T>
    operator std::shared_ptr<T> () { return std::static_pointer_cast<T>(d); }
    
    /// no implicit type conversion [for the most part], easier to spot bugs this way
    /// strict/non-strict modes should be appropriate in the future but its probably too much now
    operator         Fn&() { return fn; }
  //operator   FnFilter&() { return ff; }
    operator      int8_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return int8_t(*v.n_value.vref);
        return *((  int8_t *)v.n);
    }
    operator     uint8_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint8_t(*v.n_value.vref);
        return *(( uint8_t *)v.n);
    }
    operator     int16_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return *(( int16_t *)v.n);
    }
    operator    uint16_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return *((uint16_t *)v.n);
    }
    operator     int32_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return int32_t(*v.n_value.vref);
        if (v.t == Type::Str)
            return v.s.integer();
        return *(( int32_t *)v.n);
    }
    operator    uint32_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint32_t(*v.n_value.vref);
        return *((uint32_t *)v.n);
    }
    operator     int64_t() {
        if (t == Type::Ref)
            return int64_t(*n_value.vref);
        return *(( int64_t *)n);
    }
    operator    uint64_t() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref)
            return uint64_t(*v.n_value.vref);
        return *((uint64_t *)v.n);
    }
    operator       void*() {
        var &v = var::resolve(*this);
        assert(v.t == Type::Ref);
        return v.n;
    };
    operator       node*() {
        var &v = var::resolve(*this);
        assert(v.t == Type::Ref);
        return (node*)v.n;
    };
    operator       float() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref) return float(*v.n_value.vref);
        assert( v.t == Type::f32 || v.t == Type::f64);
        return (v.t == Type::f32) ? *(float *)v.n :
               (v.t == Type::f64) ?   float(*(double *)v.n) :
               (v.t == Type::Str) ?   float(v.s.real()) : 0.0f;
    }
    operator      double() {
        var &v = var::resolve(*this);
        if (v.t == Type::Ref) return double(*v.n_value.vref);
        return (v.t == Type::f64) ? *(double *)v.n :
               (v.t == Type::f32) ?   double(*(float *)v.n) :
               (v.t == Type::Str) ? v.s.real() : 0.0;
    }
           operator string();
           operator bool();
    
    bool operator!();
    
    template <typename T>
    T *data() {
        var &v = var::resolve(*this);
        //v.compact();
        return (T *)v.d.get();
    }
    
       void reserve(size_t sz);
       Size    size();
       void    copy(var &ref);
       var     copy();
      
               ~var();
                var(const var &ref);
    
    bool operator!=(const var &ref);
    bool operator==(const var &ref);
    bool operator==(var &ref);
    var& operator= (const var &ref);
    bool operator==(::map<string, var> &m);
    bool operator!=(::map<string, var> &m);
    bool operator==(uint16_t v);
    bool operator==( int16_t v);
    bool operator==(uint32_t v);
    bool operator==( int32_t v);
    bool operator==(uint64_t v);
    bool operator==( int64_t v);
    bool operator==(   float v);
    bool operator==(  double v);
    void operator+=(     var d);
    
    static var parse_json(str &js);

    //template <typename T>
    //operator T *() {
    //    return data<T>();
    //}
        
    void notify_insert();
    void notify_delete();
    void notify_update();
    
    template <typename T>
    var& operator=(::array<T> aa) {
        m = null;
        t = Type::Array;
        c = Type::Any;
        Type::Specifier last_type = Type::Specifier(-1);
        int types = 0;
        a = ::array<var>(aa.size());
        for (auto &v: aa) {
            var d = var(v);
            if (d.t != last_type) {
                last_type  = Type::Specifier(size_t(d.t));
                types++;
            }
            a += v;
        }
        if (types == 1)
            c = last_type;
        
        notify_update();
        return *this;
    }
    
    template <typename T>
    var& operator=(::map<string, T> mm) {
        a = null; // so much neater when the smart pointers dont show. hide those nerds
        m = null;
        for (auto &[k, v]: mm)
            m[k] = v;
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
    
    void             compact();
    static std::shared_ptr<uint8_t> encode(const char *data, size_t len);
    static std::shared_ptr<uint8_t> decode(const char *b64,  size_t b64_len, size_t *alloc);
    ///
    static bool   is_numeric(const char *s);
    static str parse_numeric(char **cursor);
    static str  parse_quoted(char **cursor, size_t max_len = 0);
    void                each(FnEach each);
    void                each(FnArrayEach each);
    var         select_first(std::function<var(var &)> fn);
    ::array<var>        select(std::function<var(var &)> fn);
    static        str format(str t, ::array<var> p) {
        for (size_t k = 0; k < p.size(); k++) {
            str  p0 = str("{") + str(std::to_string(k)) + str("}");
            str  p1 = string(p[k]);
            t       = t.replace(p0, p1);
        }
        return t;
    }
};

/// Map is var:map, thats simple enough.
struct Map:var {
    ///
    typedef std::vector<pair<string,var>> vpairs;
    static typename vpairs::iterator iterator;
    
    /// default
    Map(::map<string, var> m = {}) : var(m)  { }
    Map(nullptr_t n)               : var(::map<string, var>()) { }
    
    /// map from args
    Map(int argc, cchar_t* argv[], Map def = {}, string field_field = "");
    
    /// map from initializer list
    Map(std::initializer_list<pair<string, var>> args) : var(::map<string, var>()) {
        m = ::map<string, var>(args.size());
        for (auto &[k,v]: args)
            m[k ] = v;
    }
    
    /// assignment op and copy constructor
    Map(const Map &m) :  var(m.m) { }
    Map(const var &m) :  var(m.m) { }
    Map &operator=(const Map &ref) {
        if (this != &ref) {
            t = ref.t;
            c = ref.c;
            m = ref.m;
        }
        return *this;
    }
   ~Map() { }

    ///
    inline var              &back() { return m.pairs->back().value; }
    inline var             &front() { return m.pairs->front().value; }
    inline typeof(iterator) begin() { return m.begin(); }
    inline typeof(iterator)   end() { return m.end();   }
};

/// we need the distinction that a var is a Ref type on its type so member specialization can happen, keeping the internals of var.
struct Ref:var {
    Ref():var(Type::Ref) { }
    Ref(var &v) {
        t = Type::Ref;
        n_value.vref = &var::resolve(v);
    }
};
