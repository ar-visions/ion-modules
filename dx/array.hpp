#pragma once
#include <dx/io.hpp>
#include <dx/var.hpp>
#include <vector>
#include <queue>
#include <list>
#include <random>
#include <dx/rand.hpp>

struct var;
template <class T>
struct vec:io {
protected:
    static typename std::vector<T>::iterator iterator;
    std::vector<T> a;
public:
    typedef T value_type;
    
    vec(std::initializer_list<T> v) {
        for (auto &i: v)
            a.push_back(i);
    }
    vec(size_t sz, T v) {
        a.resize(sz, v);
    }
    vec(std::nullptr_t n) { }
    /*
    vec(vec<var> &d) {
        a.reserve(d.size());
        for (auto &dd: d)
            a += dd;
    }*/
    
    vec(var &d) {
        size_t sz = d.size();
        a.reserve(sz);
        if (sz) for (var &row:*d.a.get())
            a.push_back(row);
    }
    
    static vec<T> import(std::vector<T> v) {
        vec<T> a(v.size());
        for (auto &i: v)
            a += i;
        return a;
    }
    
    void resize(size_t sz, T v = T()) {
        a.resize(sz, v);
    }
    
    int count(T v) {
        int r = 0;
        for (auto &i: a)
            if (v == i)
                r++;
        return r;
    }
    
    void shuffle() {
        std::vector<std::reference_wrapper<const T>> v(a.begin(), a.end());
        std::shuffle(v.begin(), v.end(), Rand::e);
        std::vector<T> sh { v.begin(), v.end() };
        a.swap(sh);
    }
    
    vec(const T *d, size_t sz) { // todo: remove
        a.reserve(sz);
        for (size_t i = 0; i < sz; i++)
            a.push_back(d[i]);
    }
    
    operator var() {
        if constexpr (std::is_same_v<T,  int8_t> || std::is_same_v<T,  uint8_t> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> ||
                      std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
                      std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> ||
                      std::is_same_v<T,   float> || std::is_same_v<T,  double>) {
            return var(a.data(), a.size());
        }
        std::vector<var> v;
        v.reserve(a.size());
        for (auto &i:a)
            v.push_back(i);
        return v;
    }
    void clear() {
        a.clear();
    }
    void expand(size_t sz, T f)     {
        for (size_t i = 0; i < sz; i++)
            a.push_back(f);
    }
    vec()                           {                                    }
    vec(size_t z)                   { a.reserve(z);                      }
    inline T &back()                { return a.back();                   }
    inline T &front()               { return a.front();                  }
    inline std::vector<T> &vector() { return a;                          }
    inline T *data() const          { return (T *)a.data();              }
    inline T& operator[](size_t i)  { return a[i];                       }
    inline typeof(iterator) begin() { return a.begin();                  }
    inline typeof(iterator) end()   { return a.end();                    }
    inline void reserve(size_t sz)  { a.reserve(sz);                     }
    inline size_t size() const      { return a.size();                   }
    inline size_t capacity() const  { return a.capacity();               }
    inline var select_first(std::function<var(T &)> fn) const           {
        for (auto &i: a) {
            var r = fn(i);
            if (r)
                return r;
        }
        return null;
    }
    static inline vec<T> *new_import(var &a) {
        assert(a == Type::Array);
        vec<T> *v = new vec<T>(a.size());
        for (auto &i: *a.a)
            *v += T(i);
        return v;
    }
    inline void operator +=   (T v) { a.push_back(v);       }
    inline void erase(int index)    {
        if (index >= 0)
            a.erase(a.begin() + size_t(index));
    }
    inline void operator -= (int i) {
        return erase(i);
    }
    inline size_t index_of(T v) const {
        for (size_t i = 0; i < size(); i++)
            if (a[i] == v)
                return i;
        return -1;
    }
    inline operator std::vector<var>() {
        auto r = std::vector<var>();
        r.reserve(a.size());
        for (auto &i: a)
            r.push_back(var(i));
        return r;
    }
    inline bool operator==(vec<T> &b) {
        if (size() != b.size())
            return false;
        for (size_t i = 0; i < size(); i++)
            if ((*this)[i] != b[i])
                return false;
        return true;
    }
    inline bool operator!=(vec<T> &b) {
        return !operator==(b);
    }
    //inline operator T *() {
    //    return data(); /// as a boolean operation (which the compiler thinks it is?), its a bit lame
    //}
    inline operator bool() const    { return a.size() > 0;  }
    inline bool operator!() const   { return a.size() == 0; }
};

template<typename>
struct serializable              : std::true_type {};

template<typename>
struct is_vec                    : std::false_type {};

template<typename T>
struct is_vec<vec<T>>            : std::true_type  {};

template<typename>
struct is_func                   : std::false_type {};

template<typename T>
struct is_func<std::function<T>> : std::true_type  {};



template<typename>
struct is_strable                : std::false_type {};

template<> struct is_strable<str>    : std::true_type  {};



