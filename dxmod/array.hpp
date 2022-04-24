#pragma once
#include <vector>
#include <queue>
#include <list>
#include <random>
#include <filesystem>
#include <dx/rand.hpp>
#include <dx/size.hpp>

typedef std::filesystem::path path_t;

template <class T>
struct array:io {
//protected:
    static typename std::vector<T>::iterator iterator;
    typedef std::shared_ptr<std::vector<T>> vshared;
    vshared a;
    
    vshared &realloc(Size res) {
        a = vshared(new std::vector<T>());
        if (res)
            a->reserve(size_t(res));
        return a;
    }

    std::vector<T> &ref(Size res = 0) { return *(a ? a : realloc(res)); }
    
public:
    typedef T value_type;
    
/* ~array() { }
    array &operator=(array<T> &ref) {
        if (this != &ref)
            a = ref.a;
        return *this;
    }
    array(array<T> &ref) : a(ref.a) { } */
    array()                           { }
    array(std::initializer_list<T> v) { realloc(v.size());       for (auto &i: v) a->push_back(i);                    }
    array(const T *d, Size sz)        { assert(d);  realloc(sz); for (size_t i = 0; i < sz; i++) a->push_back(d[i]);  }
    array(Size sz, T v)               {             realloc(sz); for (size_t i = 0; i < sz; i++) a->push_back(v);     }
    array(int  sz, std::function<T(size_t)> fn) {   realloc(sz); for (size_t i = 0; i < sz; i++) a->push_back(fn(i)); }
    array(std::nullptr_t n)           { }
    array(Size sz)                    { realloc(sz); }
    array(std::filesystem::path path);
    
    /// quick-sort raw implementation.
    array<T> sort(std::function<int(T &a, T &b)> cmp) {
        auto   &self = ref();
        array<T *> r = {size(), [&](size_t i) { return &self[i]; }};
        std::function<void(int,   int)> qk;
            qk =       [&](int s, int e) {
            /// sanitize, set mutable vars i and j to the start and end
            if (s >= e) return;
            int i  = s, j = e, pv = s;
            ///
            while (i < j) {
                /// standard search pattern
                while (cmp(r[i], r[pv]) <= 0 && i < e)
                    i++;
                
                /// and over there too ever forget your tail? we all have
                while (cmp(r[j], r[pv]) > 0)
                    j--;
                
                /// send away team of red shirts
                if (i < j)
                    std::swap(r[i], r[j]);
            }
            /// throw a water bottle
            std::swap(r[pv], r[j]);
            
            /// rant and rave how this generation never does anything, pencil-in for interns
            qk(s, j - 1);
            qk(j + 1, e);
        };
        
        /// start the magic
        qk(0, size() - 1);
        ///
        /// realize the magic..
        return { size(), [&](size_t i) { return *self[i]; }};
    }
    
    T                   &back()              { return ref().back();                }
    T                  &front()              { return ref().front();               }
    Size                 size()        const { return a ? a->size()     : 0;       }
    Size             capacity()        const { return a ? a->capacity() : 0;       }
    std::vector<T>    &vector()              { return ref();                       }
    typeof(iterator)    begin()              { return ref().begin();               }
    typeof(iterator)      end()              { return ref().end();                 }
    T                   *data()        const { return (T *)(a ? a->data() : null); }
    void      operator    += (T v)           { ref().push_back(v);                 }
    void      operator    -= (int i)         { return erase(i);                    }
              operator   bool()        const { return a && a->size() > 0;          }
    bool      operator      !()        const { return !(operator bool());          }
    T&        operator     [](size_t i)      { return ref()[i];                    }
    bool      operator!=     (array<T> b)    { return (operator==(b));             }
    ///
    bool      operator==     (array<T> b)    {
        if (!a && !b.a)
            return true;
        size_t sz = size();
        bool sz_equal = sz == b.size();
        if ( sz_equal && sz ) {
            T *a_v = (T *)a->data();
            T *b_v = (T *)b.data();
            for (size_t i = 0; i < size(); i++)
                if (!(a_v[i] == b_v[i]))
                    return false;
        }
        return sz_equal;
    }
    ///
    array<T> &operator=(const array<T> &ref) {
        if (this != &ref)
            a = ref.a;
        return *this;
    }
    ///
    void   expand(Size sz, T f)         { for (size_t i = 0; i < sz; i++) a->push_back(f); }
    void   erase(int index)             { if (index >= 0) a->erase(a->begin() + size_t(index)); }
    void   reserve(size_t sz)           { ref().reserve(sz); }
    void   clear(size_t sz = 0)         { ref().clear(); if (sz) reserve(sz); }
    void   resize(size_t sz, T v = T()) { ref().resize(sz, v); }
    ///
    static array<T> import(std::vector<T> v) {
        array<T> a(v.size());
        for (auto &i: v)
            a += i;
        return a;
    }
    
    template <typename R>
    array<R> convert() {
        array<R> out(a->size());
        if (a) for (auto &i: *a)
            out += i;
        return out;
    }
    
    int count(T v) const {
        int r = 0;
        if (a) for (auto &i: *a)
            if (v == i)
                r++;
        return r;
    }
    
    void shuffle() {
        std::vector<std::reference_wrapper<const T>> v(a->begin(), a->end());
        std::shuffle(v.begin(), v.end(), Rand::global_engine());
        std::vector<T> sh { v.begin(), v.end() };
        a->swap(sh);
    }
    
    /// its a vector operation it deserves to expand
    template <typename R>
    R select_first(std::function<R(T &)> fn) const {
        if (a) for (auto &i: *a) {
            R r = fn((T &)i);
            if (r)
                return r;
        }
        return R(null);
    }

    /// todo: all, all delim use int, all. not a single, single one use size_t.
    int index_of(T v) const { 
        int index = 0;
        if (a) for (auto &i: *a) {
            if (i == v)
                return index;
            index++;
        }
        return -1;
    }
};

// shape typed by stride, i think its nice to declare that
template <const Stride S>
struct Shape:Size {
    typedef std::vector<ssize_t> VShape;
    static typename VShape::iterator iterator;
    ///
    VShape shape;
    Stride stride = S;
    ///
    int      dims() const { return shape.size(); }
    size_t   size() const {
        ssize_t  r    = 0;
        bool    first = true;
        ///
        for (auto s:shape)
            if (first) {
                first = false;
                r     = s;
            } else
                r    *= s;
        return size_t(r);
    }
    
    /// switching to ssize_t after its established
    size_t operator [] (size_t i) { return size_t(shape[i]); }
    ///
    Shape(::array<int> sz)                : shape(sz) { }
    Shape(int d0, int d1, int d2, int d3) : shape({ d0, d1, d2, d3 }) { }
    Shape(int d0, int d1, int d2)         : shape({ d0, d1, d2     }) { }
    Shape(int d0, int d1)                 : shape({ d0, d1         }) { }
    Shape(int d0)                         : shape({ d0             }) { }
    Shape()                               { }
    ///
    Shape(const Size &ref)                : shape(std::vector<ssize_t> { ssize_t((Size &)ref) }) { }
    ///
    static Shape<S> rgba(int w, int h) {
        if constexpr (S == Major)
            return { h, w, 4 };
        assert(false);
        return { h, 4, w }; /// decision needed on this
    };
    ///
    inline typeof(iterator) begin() { return shape.begin();    }
    inline typeof(iterator) end()   { return shape.end();      }
};
  
template<typename>
struct serializable              : std::true_type {};

template<typename>
struct is_vec                    : std::false_type {};

template<typename T>
struct is_vec<array<T>>          : std::true_type  {};

template <typename T>
bool equals(T v, array<T> values) { return values.index_of(v) >= 0; }

template <typename T>
bool isnt(T v, array<T> values)   { return !equals(v, values); }


