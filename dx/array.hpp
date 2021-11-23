#pragma once
#include <dx/var.hpp>
#include <vector>
#include <queue>
#include <list>
#include <random>
#include <dx/rand.hpp>

struct var;
template <class T>
struct vec {
protected:
    static typename std::vector<T>::iterator iterator;
    std::vector<T> a;
public:
    vec(std::initializer_list<T> v) {
        for (auto &i: v)
            a.push_back(i);
    }
    vec(nullptr_t n) { }
    /*
    vec(vec<var> &d) {
        a.reserve(d.size());
        for (auto &dd: d)
            a += dd;
    }*/
    static vec<T> import(std::vector<T> v) {
        vec<T> a(v.size());
        for (auto &i: v)
            a += i;
        return a;
    }
    void resize(size_t sz) {
        a.resize(sz);
    }
    int count(T v) {
        int ret = 0;
        for (auto &i: a)
            if (v == i)
                ret++;
        return ret;
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
    inline vec<T> &fill(T v)           {
        assert(a.size()    == 0);
        assert(a.capacity() > 0);
        for (size_t i = 0; i < capacity(); i++)
            *this += v;
        return *this;
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
    inline var query_first(std::function<var(T &)> fn) const           {
        for (auto &i: a) {
            var r = fn(i);
            if (r)
                return r;
        }
        return null;
    }
    static inline vec<T> import(var &a) {
        assert(a == var::Array);
        vec<T> v(a.size());
        for (auto &i: *a.a)
            v += i;
        return v;
    }
    inline void operator +=   (T v) { a.push_back(v);                    }
    inline void operator -= (int i) { if (i >= 0)
                                         a.erase(a.begin() + size_t(i)); }
    inline operator bool() const    { return a.size() > 0;               }
    inline bool operator!() const   { return a.size() == 0;              }
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
    inline operator T *() {
        return data();
    }
};
