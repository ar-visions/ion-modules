#pragma once

#include <dx/array.hpp>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <dx/var.hpp>

template <typename T>
struct Vector {
    operator bool()  { return !std::isnan(*(T *)this); }
    bool operator!() { return !(operator bool());      }
    inline T &operator[](size_t i) const {
        T *v = (T *)this;
        return v[i];
    }
};

template <typename T>
struct Vec4;

template <typename T>
struct Vec2: Vector<T> {
    alignas(T) T x, y;
    Vec2(nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
    Vec2(T x)      : x(x), y(x) { }
    Vec2(T x, T y) : x(x), y(y) { }
    
    Vec4<T> xxyy();
    
    static Vec2<T> &null() {
        static Vec2<T> *n = nullptr;
        if (!n)
             n = new Vec2<T>(nullptr);
        return *n;
    }
    
    operator size_t() const {
        return size_t(x) * size_t(y);
    }
    
    Vec2(var &d) {
        if (d.size() < 2) {
            x = std::numeric_limits<T>::quiet_NaN();
        } else if (d == var::Array) {
            x = T(d[size_t(0)]);
            y = T(d[size_t(1)]);
        } else if (d == var::Map) {
            x = T(d["x"]);
            y = T(d["y"]);
        } else {
            assert(false);
        }
    }
    
    static void aabb(vec<Vec2<T>> &a, Vec2<T> &v_min, Vec2<T> &v_max) {
        const int sz = a.size();
        v_min        = a[0];
        v_max        = a[0];
        for (int i   = 1; i < sz; i++) {
            auto &v  = a[i];
            v_min    = v_min.min(v);
            v_max    = v_max.max(v);
        }
    }
    
    inline Vec2<T> abs() {
        if constexpr (std::is_same_v<T, int>)
            return *this;
        return Vec2<T> { std::abs(x), std::abs(y) };
    }
    
    inline Vec2<T> floor() {
        if constexpr (std::is_same_v<T, int>)
            return *this;
        return Vec2<T> { std::floor(x), std::floor(y) };
    }
    
    inline Vec2<T> ceil() {
        if constexpr (std::is_same_v<T, int>)
            return *this;
        return Vec2<T> { std::ceil(x),  std::ceil(y) };
    }
    
    inline Vec2<T> clamp(Vec2<T> mn, Vec2<T> mx) {
        return Vec2<T> {
            std::clamp(x, mn.x, mx.x),
            std::clamp(y, mn.y, mx.y)
        };
    }
    
    inline T max() {
        return std::max(x, y);
    }
    
    inline T min() {
        return std::min(x, y);
    }
    
    inline Vec2<T> max(T v) {
        return Vec2<T> {
            std::max(x, v),
            std::max(y, v)
        };
    }
    
    inline Vec2<T> min(T v) {
        return Vec2<T> {
            std::min(x, v),
            std::min(y, v)
        };
    }
    
    inline Vec2<T> max(Vec2<T> v) {
        return Vec2<T> {
            std::max(x, v.x),
            std::max(y, v.y)
        };
    }
    
    inline Vec2<T> min(Vec2<T> v)   {
        return Vec2<T> {
            std::min(x, v.x),
            std::min(y, v.y)
        };
    }
    
    operator var() { return std::vector<T> { x, y }; }
    
    bool operator== (const Vec2<T> &r) const { return x == r.x && y == r.y; }

    void operator += (Vec2<T>  r)   { x += r.x; y += r.y; }
    void operator -= (Vec2<T>  r)   { x -= r.x; y -= r.y; }
    void operator *= (Vec2<T>  r)   { x *= r.x; y *= r.y; }
    void operator /= (Vec2<T>  r)   { x /= r.x; y /= r.y; }
    ///
    void operator *= (T r)          { x *= r;   y *= r; }
    void operator /= (T r)          { x /= r;   y /= r; }
    ///
    Vec2<T> operator + (Vec2<T> r)  { return { x + r.x, y + r.y }; }
    Vec2<T> operator - (Vec2<T> r)  { return { x - r.x, y - r.y }; }
    Vec2<T> operator * (Vec2<T> r)  { return { x * r.x, y * r.y }; }
    Vec2<T> operator / (Vec2<T> r)  { return { x / r.x, y / r.y }; }
    ///
    Vec2<T> operator * (T r)        { return { x * r, y * r }; }
    Vec2<T> operator / (T r)        { return { x / r, y / r }; }
    ///
    operator Vec2<float>()          { return Vec2<float>  {  float(x),  float(y) }; }
    operator Vec2<double>()         { return Vec2<double> { double(x), double(y) }; }
    operator Vec2<int>()            { return Vec2<int>    {    int(x),    int(y) }; }
    ///
    static double area(vec<Vec2<T>> &p) {
        double area = 0.0;
        const int n = int(p.size());
        int       j = n - 1;
        
        // shoelace formula
        for (int i = 0; i < n; i++) {
            Vec2<T> &pj = p[j];
            Vec2<T> &pi = p[i];
            area += (pj.x + pi.x) * (pj.y - pi.y);
            j = i;
        }
     
        // return absolute value
        return std::abs(area / 2.0);
    }
};

template <typename T>
struct Vec3: Vector<T> {
    alignas(T) T x, y, z;
    Vec3(nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
    Vec3(T x) : x(x), y(x), z(x) { }
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
    
    Vec3<T> operator + (Vec3<T> &r) { return { x + r.x, y + r.y, z + r.z }; }
    Vec3<T> operator - (Vec3<T> &r) { return { x - r.x, y - r.y, z - r.z }; }
    Vec3<T> operator * (Vec3<T> &r) { return { x * r.x, y * r.y, z * r.z }; }
    Vec3<T> operator / (Vec3<T> &r) { return { x / r.x, y / r.y, z / r.z }; }
    
    Vec3<T> operator * (T r)        { return { x * r.x, y * r.y, z * r.z }; }
    Vec3<T> operator / (T r)        { return { x / r.x, y / r.y, z / r.z }; }

    inline Vec2<T> xy()             { return Vec2<T> { x, y }; }
    inline operator Vec3<float>()   { return Vec3<float>  { float(x),  float(y),  float(z)  }; }
    inline operator Vec3<double>()  { return Vec3<double> { double(x), double(y), double(z) }; }
    
    operator var() {
        return std::vector<T> { x, y, z };
    }
    
    Vec3(var &d) {
        x = T(d[size_t(0)]);
        y = T(d[size_t(1)]);
        z = T(d[size_t(2)]);
    }
    
    Vec3<T> &operator=(const std::vector<T> f) {
        assert(f.size() == 3);
        x = f[0];
        y = f[1];
        z = f[2];
        return *this;
    }
    
    T len_sqr() const {
        return x * x + y * y + z * z;
    }
    
    T len() const {
        return sqrt(len_sqr());
    }
    
    Vec3<T> normalize() const {
        T l = len();
        return *this / l;
    }
    
    inline Vec3<T> cross(Vec3<T> b) {
        Vec3<T> &a = *this;
        return { a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x };
    }
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
    Vec4(T x)                  : x(x),   y(x),   z(x),   w(x)   { }
    Vec4(T x, T y, T z, T w)   : x(x),   y(y),   z(z),   w(w)   { }
    Vec4(Vec2<T> x, Vec2<T> y) : x(x.x), y(x.y), z(y.x), w(y.y) { }
    
    static Vec4<T> &null() {
        static Vec4<T> *n = nullptr;
        if (!n)
             n = new Vec4<T>(nullptr);
        return *n;
    }

    bool operator== (const Vec4<T> &r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }

    void operator += (Vec4<T> &r)          { x += r.x; y += r.y; z += r.z; w += r.w; }
    void operator -= (Vec4<T> &r)          { x -= r.x; y -= r.y; z -= r.z; w -= r.w; }
    void operator *= (Vec4<T> &r)          { x *= r.x; y *= r.y; z *= r.z; w *= r.w; }
    void operator /= (Vec4<T> &r)          { x /= r.x; y /= r.y; z /= r.z; w /= r.w; }
    
    inline void operator *= (T r)          { x *= r;   y *= r;   z *= r;   w *= r; }
    inline void operator /= (T r)          { x /= r;   y /= r;   z /= r;   w /= r; }
    
    inline Vec4<T> operator + (Vec4<T>  r) { return { x + r.x, y + r.y, z + r.z, w + r.z }; }
    inline Vec4<T> operator - (Vec4<T>  r) { return { x - r.x, y - r.y, z - r.z, z - r.z }; }
    inline Vec4<T> operator * (Vec4<T>  r) { return { x * r.x, y * r.y, z * r.z, z * r.z }; }
    inline Vec4<T> operator / (Vec4<T>  r) { return { x / r.x, y / r.y, z / r.z, z / r.z }; }
    
    inline Vec4<T> operator * (T v)        { return { x * v, y * v, z * v, w * v }; }
    inline Vec4<T> operator / (T v)        { return { x / v, y / v, z / v, z / v }; }
    
    inline Vec3<T> xyz()                   { return Vec3<T> { x, y, z }; }
    
    inline operator Vec4<float>()          { return Vec4<float>  { float(x),  float(y),  float(z),  float(w)  }; }
    inline operator Vec4<double>()         { return Vec4<double> { double(x), double(y), double(z), double(w) }; }
    
    inline Vec2<T> xy()                    { return Vec2<T> { x, y }; }
    inline Vec2<T> xz()                    { return Vec2<T> { x, z }; }
    inline Vec2<T> yz()                    { return Vec2<T> { y, z }; }
    inline Vec2<T> xw()                    { return Vec2<T> { x, w }; }
    inline Vec2<T> yw()                    { return Vec2<T> { y, w }; }
};

typedef map<std::string, var> Args; // todo: why here

template <typename T>
struct Edge {
    Vec2<T> &a;
    Vec2<T> &b;
    
    /// returns x-value of intersection of two lines
    inline T x(Vec2<T> c, Vec2<T> d) {
        T num = (a.x*b.y - a.y*b.x) * (c.x-d.x) - (a.x-b.x) * (c.x*d.y - c.y*d.x);
        T den = (a.x-b.x) * (c.y-d.y) - (a.y-b.y) * (c.x-d.x);
        return num / den;
    }
    
    /// returns y-value of intersection of two lines
    inline T y(Vec2<T> c, Vec2<T> d) {
        T num = (a.x*b.y - a.y*b.x) * (c.y-d.y) - (a.y-b.y) * (c.x*d.y - c.y*d.x);
        T den = (a.x-b.x) * (c.y-d.y) - (a.y-b.y) * (c.x-d.x);
        return num / den;
    }
    
    /// returns xy-value of intersection of two lines
    inline Vec2<T> xy(Vec2<T> c, Vec2<T> d) {
        return { x(c, d), y(c, d) };
    }
};

template <typename T>
struct Rect: Vector<T> {
    alignas(T) T x, y, w, h;
    Rect(nullptr_t n = nullptr) : x(std::numeric_limits<T>::quiet_NaN()) { }
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
    bool contains(Vec2<T> p) const {
        return p.x >= x && p.x <= (x + w) &&
               p.y >= y && p.y <= (y + h);
    }
    bool    operator== (const Rect<T> &r) { return x == r.x && y == r.y && w == r.w && h == r.h; }
    bool    operator!= (const Rect<T> &r) { return !operator==(r); }
    Rect<T> operator * (T r) { return { x * r, y * y, w * r, h * r }; }
    Rect<T> operator / (T r) { return { x / r, y / y, w / r, h / r }; }
    operator Rect<int>()     { return {    int(x),    int(y),    int(w),    int(h) }; }
    operator Rect<float>()   { return {  float(x),  float(y),  float(w),  float(h) }; }
    operator Rect<double>()  { return { double(x), double(y), double(w), double(h) }; }

    operator var() {
        return std::vector<T> { x, y, w, h };
    }
    
    Rect<T>(var &d) {
        if (d.size()) {
            x = T(d[size_t(0)]);
            y = T(d[size_t(1)]);
            w = T(d[size_t(2)]);
            h = T(d[size_t(3)]);
        } else
            x = std::numeric_limits<double>::quiet_NaN();
    }
    
    vec<Vec2<T>> edges() const {
        return {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
    }
    
    vec<Vec2<T>> clip(vec<Vec2<T>> &poly) const {
        const Rect<T>  &clip = *this;
        vec<Vec2<T>>       p = poly;
        vec<Vec2<T>>       e = clip.edges();
        for (int i = 0; i < e.size(); i++) {
            Vec2<T>  &e0 = e[i];
            Vec2<T>  &e1 = e[(i + 1) % e.size()];
            Edge<T> edge = { e0, e1 };
            vec<Vec2<T>> cl;
            for (int ii = 0; ii < p.size(); ii++) {
                const Vec2<T> &pi = p[(ii + 0)];
                const Vec2<T> &pk = p[(ii + 1) % p.size()];
                const bool    top = i == 0;
                const bool    bot = i == 2;
                const bool    lft = i == 3;
                if (top || bot) {
                    const bool cci = top ? (edge.a.y <= pi.y) : (edge.a.y > pi.y);
                    const bool cck = top ? (edge.a.y <= pk.y) : (edge.a.y > pk.y);
                    if (cci) {
                        cl     += cck ? pk : Vec2<T> { edge.x(pi, pk), edge.a.y };
                    } else if (cck) {
                        cl     += Vec2<T> { edge.x(pi, pk), edge.a.y };
                        cl     += pk;
                    }
                } else {
                    const bool cci = lft ? (edge.a.x <= pi.x) : (edge.a.x > pi.x);
                    const bool cck = lft ? (edge.a.x <= pk.x) : (edge.a.x > pk.x);
                    if (cci) {
                        cl     += cck ? pk : Vec2<T> { edge.a.x, edge.y(pi, pk) };
                    } else if (cck) {
                        cl     += Vec2<T> { edge.a.x, edge.y(pi, pk) };
                        cl     += pk;
                    }
                }
            }
            p = cl;
        }
        return p;
    }
};

#if INTPTR_MAX == INT64_MAX
typedef double       real;
#elif INTPTR_MAX == INT32_MAX
typedef float        real;
#else
#error Unknown pointer size or missing size macros!
#endif

typedef float        r32;
typedef double       r64;

typedef Vec2<float>  vec2f;
typedef Vec2<real>   vec2;
typedef Vec2<double> vec2d;
typedef Vec3<float>  vec3f;
typedef Vec3<real>   vec3;
typedef Vec3<double> vec3d;
typedef Vec4<float>  vec4f;
typedef Vec4<real>   vec4;
typedef Vec4<double> vec4d;

typedef Vec2<int>    vec2i;
typedef Vec3<int>    vec3i;
typedef Vec4<int>    vec4i;

typedef Rect<double> rectd;
typedef Rect<float>  rectf;
typedef Rect<int>    recti;

template <typename T>
static inline T mix(T a, T b, T i) {
    return (a * (1.0 - i)) + (b * i);
}

template <typename T>
static inline Vec2<T> mix(Vec2<T> a, Vec2<T> b, T i) {
    return (a * (1.0 - i)) + (b * i);
}

template <typename T>
static inline Vec3<T> mix(Vec3<T> a, Vec3<T> b, T i) {
    return (a * (1.0 - i)) + (b * i);
}

template <typename T>
static inline Vec4<T> mix(Vec4<T> a, Vec4<T> b, T i) {
    return (a * (1.0 - i)) + (b * i);
}

template <typename T>
static inline Vec4<T> xxyy(Vec2<T> v) { return { v.x, v.x, v.y, v.y }; }

template <typename T>
static inline Vec4<T> xxyy(Vec3<T> v) { return { v.x, v.x, v.y, v.y }; }

template <typename T>
static inline Vec4<T> xxyy(Vec4<T> v) { return { v.x, v.x, v.y, v.y }; }

template <typename T>
static inline Vec4<T> ywyw(Vec4<T> v) { return { v.y, v.x, v.y, v.w }; }

template <typename T>
static inline Vec4<T> xyxy(Vec2<T> v) { return { v.x, v.y, v.x, v.y }; }

template <typename T>
static inline Vec2<T> fract(Vec2<T> v) { return v - v.floor(); }

template <typename T>
static inline Vec3<T> fract(Vec3<T> v) { return v - v.floor(); }

template <typename T>
static inline Vec4<T> fract(Vec4<T> v) { return v - v.floor(); }

