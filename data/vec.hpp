#pragma once
#include <data/data.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <data/data.hpp>

template <typename T>
struct Vector {
    operator bool()  { return !std::isnan(*(T *)this); }
    bool operator!() { return !(operator bool());      }
};

template <typename T>
struct Vec2: Vector<T> {
    alignas(T) T x, y;
    Vec2(nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
    Vec2(T x, T y) : x(x), y(y) { }
    
    static Vec2<T> &null() {
        static Vec2<T> *n = nullptr;
        if (!n)
             n = new Vec2<T>(nullptr);
        return *n;
    }
    
    Vec2(Data &d) {
        if (d.size() < 2) {
            x = std::numeric_limits<T>::quiet_NaN();
        } else if (d.t == Data::Array) {
            x = T(d[size_t(0)]);
            y = T(d[size_t(1)]);
        } else if (d.t == Data::Map) {
            x = T(d["x"]);
            y = T(d["y"]);
        } else {
            assert(false);
        }
    }
    
    operator Data() {
        return std::vector<T> { x, y };
    }

    bool operator== (const Vec2<T> &r) const { return x == r.x && y == r.y; }

    void operator += (Vec2<T>  r)   { x += r.x; y += r.y; }
    void operator -= (Vec2<T>  r)   { x -= r.x; y -= r.y; }
    void operator *= (Vec2<T>  r)   { x *= r.x; y *= r.y; }
    void operator /= (Vec2<T>  r)   { x /= r.x; y /= r.y; }
    
    void operator *= (T r)          { x *= r;   y *= r; }
    void operator /= (T r)          { x /= r;   y /= r; }
    
    Vec2<T> operator + (Vec2<T> r)  { return { x + r.x, y + r.y }; }
    Vec2<T> operator - (Vec2<T> r)  { return { x - r.x, y - r.y }; }
    Vec2<T> operator * (Vec2<T> r)  { return { x * r.x, y * r.y }; }
    Vec2<T> operator / (Vec2<T> r)  { return { x / r.x, y / r.y }; }
    
    Vec2<T> operator * (T r)        { return { x * r, y * r.y }; }
    Vec2<T> operator / (T r)        { return { x / r, y / r.y }; }
    
    operator Vec2<float>() { return Vec2<float> { float(x), float(y) }; }
};

template <typename T>
struct Vec3: Vector<T> {
    alignas(T) T x, y, z;
    Vec3(nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
    Vec3(T x, T y, T z) : x(x), y(y), z(z) { }
    
    static Vec3<T> &null() {
        static Vec3<T> *n = nullptr;
        if (!n)
             n = new Vec3<T>(nullptr);
        return *n;
    }

    bool operator== (const Vec3<T> &r) const { return x == r.x && y == r.y && z == r.z; }

    void operator += (Vec3<T> &r)   { x += r.x; y += r.y; z += r.z; }
    void operator -= (Vec3<T> &r)   { x -= r.x; y -= r.y; z -= r.z; }
    void operator *= (Vec3<T> &r)   { x *= r.x; y *= r.y; z *= r.z; }
    void operator /= (Vec3<T> &r)   { x /= r.x; y /= r.y; z /= r.z; }
    
    void operator *= (T r)          { x *= r;   y *= r;   z *= r.z; }
    void operator /= (T r)          { x /= r;   y /= r;   z /= r.z; }
    
    Vec2<T> operator + (Vec3<T> &r) { return { x + r.x, y + r.y, z + r.z }; }
    Vec2<T> operator - (Vec3<T> &r) { return { x - r.x, y - r.y, z - r.z }; }
    Vec2<T> operator * (Vec3<T> &r) { return { x * r.x, y * r.y, z * r.z }; }
    Vec2<T> operator / (Vec3<T> &r) { return { x / r.x, y / r.y, z / r.z }; }
    
    Vec2<T> operator * (T r)        { return { x * r, y * r.y, z * r.z }; }
    Vec2<T> operator / (T r)        { return { x / r, y / r.y, z / r.z }; }
    
    Vec2<T> xy()                    { return Vec2<T> { x, y }; }
    
    operator Vec3<float>()          { return Vec3<float>  { float(x),  float(y),  float(z)  }; }
    operator Vec3<double>()         { return Vec3<double> { double(x), double(y), double(z) }; }
};


template <typename T>
struct Vec4: Vector<T> {
    alignas(T) T x, y, z, w;
    Vec4(nullptr_t n = nullptr) {
        if constexpr (std::is_same_v<T, int>)
            x = std::numeric_limits<T>::quiet_NaN();
        else
            x = std::numeric_limits<T>::quiet_NaN();
    }
    Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
    
    static Vec4<T> &null() {
        static Vec4<T> *n = nullptr;
        if (!n)
             n = new Vec4<T>(nullptr);
        return *n;
    }

    bool operator== (const Vec4<T> &r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }

    void operator += (Vec4<T> &r)   { x += r.x; y += r.y; z += r.z; w += r.w; }
    void operator -= (Vec4<T> &r)   { x -= r.x; y -= r.y; z -= r.z; w -= r.w; }
    void operator *= (Vec4<T> &r)   { x *= r.x; y *= r.y; z *= r.z; w *= r.w; }
    void operator /= (Vec4<T> &r)   { x /= r.x; y /= r.y; z /= r.z; w /= r.w; }
    
    void operator *= (T r)          { x *= r;   y *= r;   z *= r;   w *= r.w; }
    void operator /= (T r)          { x /= r;   y /= r;   z /= r;   w /= r.w; }
    
    Vec2<T> operator + (Vec4<T> &r) { return { x + r.x, y + r.y, z + r.z, w + r.z }; }
    Vec2<T> operator - (Vec4<T> &r) { return { x - r.x, y - r.y, z - r.z, z - r.z }; }
    Vec2<T> operator * (Vec4<T> &r) { return { x * r.x, y * r.y, z * r.z, z * r.z }; }
    Vec2<T> operator / (Vec4<T> &r) { return { x / r.x, y / r.y, z / r.z, z / r.z }; }
    
    Vec2<T> operator * (T r)        { return { x * r, y * r.y, z * r.z, w * r.w }; }
    Vec2<T> operator / (T r)        { return { x / r, y / r.y, z / r.z, z / r.z }; }
    
    Vec2<T> xy()                    { return Vec2<T> { x, y }; }
    Vec3<T> xyz()                   { return Vec3<T> { x, y, z }; }
    
    operator Vec4<float>()  { return Vec4<float>  { float(x),  float(y),  float(z),  float(w)  }; }
    operator Vec4<double>() { return Vec4<double> { double(x), double(y), double(z), double(w) }; }
};

typedef map<std::string, Data> Args;

template <typename T>
struct Rect: Vector<T> {
    alignas(T) T x, y, w, h;
    Rect(nullptr_t n = nullptr) : x(std::numeric_limits<double>::quiet_NaN()) { }
    Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) { }
    Rect(Vec2<T> p, Vec2<T> s, bool c) {
        if (c)
            *this = { p.x - s.x / 2, p.y - s.y / 2, s.x, s.y };
        else
            *this = { p.x, p.y, s.x, s.y };
    }
    Rect(Vec2<T> p0, Vec2<T> p1) {
        auto x0 = std::min(p0.x, p1.x);
        auto y0 = std::min(p0.y, p0.y);
        auto x1 = std::max(p1.x, p1.x);
        auto y1 = std::max(p1.y, p1.y);
        *this   = { x0, y0, x1 - x0, y1 - y0 };
    }
    
    static Rect<T> &null() {
        static Rect<T> *n = nullptr;
        if (!n)
             n = new Rect<T>(nullptr);
        return *n;
    }
    
    Vec2<T> size()   const { return { w, h }; }
    Vec2<T> xy()     const { return { x, y }; }
    Vec2<T> center() const { return { x + w / 2.0, y + h / 2.0 }; }
    
    bool    operator== (const Rect<T> &r) { return x == r.x && y == r.y && w == r.w && h == r.h; }
    bool    operator!= (const Rect<T> &r) { return !operator==(r); }
    Rect<T> operator * (T r) { return { x * r, y * y, w * r, h * r }; }
    Rect<T> operator / (T r) { return { x / r, y / y, w / r, h / r }; }
    operator Rect<int>()     { return {    int(x),    int(y),    int(w),    int(h) }; }
    operator Rect<float>()   { return {  float(x),  float(y),  float(w),  float(h) }; }
    operator Rect<double>()  { return { double(x), double(y), double(w), double(h) }; }
    operator Args()          { return Args {{"x",T(x)}, {"y",T(y)}, {"w",T(w)}, {"h",T(h)}}; }
};

typedef Vec2<float>  vec2f;
typedef Vec2<double> vec2;
typedef Vec3<float>  vec3f;
typedef Vec3<double> vec3;
typedef Vec4<float>  vec4f;
typedef Vec4<double> vec4;

typedef Vec2<int>    vec2i;
typedef Vec3<int>    vec3i;
typedef Vec4<int>    vec4i;

typedef Rect<double> rectd;
typedef Rect<float>  rectf;
typedef Rect<int>    recti;
