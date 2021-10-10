#pragma once
#include <data/data.hpp>
#include <vector>
#include <queue>

struct Data;
template <class T>
class vec {
    static typename std::vector<T>::iterator iterator;
    std::vector<T> a;
public:
    vec(nullptr_t n) { }
    vec(vec<Data> &d) { // todo: remove
        a.reserve(d.size());
        for (auto &dd: d)
            a += dd;
    }
    vec(std::initializer_list<T> list) {
        for (auto &v: list)
            a.push_back(v);
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
    operator Data() {
        if constexpr (std::is_same_v<T,  int8_t> || std::is_same_v<T,  uint8_t> ||
                      std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> ||
                      std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
                      std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> ||
                      std::is_same_v<T,   float> || std::is_same_v<T,  double>) {
            return Data(a.data(), a.size());
        }
        std::vector<Data> v;
        v.reserve(a.size());
        for (auto &i:a) {
            Data d = Data(i);
            v.push_back(d);
        }
        return v;
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
    inline T *data()                { return a.data();                   }
    inline T& operator[](size_t i)  { return a[i];                       }
    inline typeof(iterator) begin() { return a.begin();                  }
    inline typeof(iterator) end()   { return a.end();                    }
    inline void reserve(size_t sz)  { a.reserve(sz);                     }
    inline size_t size() const      { return a.size();                   }
    inline size_t capacity() const  { return a.capacity();               }
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
    inline operator std::vector<Data>() {
        auto r = std::vector<Data>();
        r.reserve(a.size());
        for (auto &i: a)
            r.push_back(Data(i));
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
};
