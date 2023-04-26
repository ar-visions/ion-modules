#pragma once
#include <core/core.hpp>

template <typename T>
struct vector2:mx {
    using v2 = v2<T>;
    v2 &data;
    
    ctr(vector2, mx, v2, data);
    
    inline vector2(T x, T y) : vector2(v2 { x, y }) { }
    
    inline T*                     vdata() const { return data.vdata();     }
    inline T               dot(vector2 v) const { return data.dot();       }
    real                            dot() const { return data.dot();       }
    real                           area() const { return data.area();      }
    real                            len() const { return data.len();       }
    vector2                   normalize() const { return data.normalize(); }
    inline vector2 operator+  (vector2 v) const { return data + v.data;    }
    inline vector2 operator-  (vector2 v) const { return data - v.data;    }
    inline vector2 operator*  (vector2 v) const { return data * v.data;    }
    inline vector2 operator/  (vector2 v) const { return data / v.data;    }
    inline vector2 operator*  (T       v) const { return data * v;         }
    inline vector2 operator/  (T       v) const { return data / v;         }

    inline vector2 &operator+=(vector2 v) { data += v; return *this; }
    inline vector2 &operator-=(vector2 v) { data -= v; return *this; }
    inline vector2 &operator*=(vector2 v) { data *= v; return *this; }
    inline vector2 &operator/=(vector2 v) { data /= v; return *this; }
    inline vector2 &operator*=(T       v) { data *= v; return *this; }
    inline vector2 &operator/=(T       v) { data /= v; return *this; }

    inline bool operator >= (v2 &v) const { return data >= v; }
    inline bool operator <= (v2 &v) const { return data <= v; }
    inline bool operator >  (v2 &v) const { return data >  v; }
    inline bool operator <  (v2 &v) const { return data <  v; }

};

template <typename T>
struct vector3:mx {
    using v3 = v3<T>;
    v3 &data;

    ctr(vector3, mx, v3, data);
    
    inline vector3(T x, T y, T z) : vector3(v3 { x, y, z }) { }

    inline T       dot(vector3 v)   const { return data.dot(v);      }
    inline T*      vdata()          const { return data.vdata();     }
    real           len_sqr()        const { return data.len_sqr();   }
    real           len()            const { return data.len();       }
    vector3        normalize()      const { return data.normalize(); }

    inline vector3 cross(vector3 b) const {
        v3 &a = *this;
        return { a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x };
    }

    inline vector3 operator+  (v3 v) const { return data + v.data; }
    inline vector3 operator-  (v3 v) const { return data - v.data; }
    inline vector3 operator*  (v3 v) const { return data * v.data; }
    inline vector3 operator/  (v3 v) const { return data / v.data; }
    inline vector3 operator*  (T  v) const { return data * v;      }
    inline vector3 operator/  (T  v) const { return data / v;      }

    inline vector3 &operator+=(v3 v)       { data += v; return *this; }
    inline vector3 &operator-=(v3 v)       { data -= v; return *this; }
    inline vector3 &operator*=(v3 v)       { data *= v; return *this; }
    inline vector3 &operator/=(v3 v)       { data /= v; return *this; }
    inline vector3 &operator*=(T  v)       { data *= v; return *this; }
    inline vector3 &operator/=(T  v)       { data /= v; return *this; }
};

#define ctr2(C, B, m_type, m_access) \
    mx_basics(C, B, m_type, m_access)\
    inline C(memory*     mem) : B(mem), m_access(mx::ref<m_type>()) { }\
    inline C(null_t =   null) : C(mx::alloc<C>( )) { }\
    inline C(m_type    tdata) : C(mx::alloc<C>( )) {\
            (&m_access) -> ~DC();\
        new (&m_access)     DC(tdata);\
    }

template <typename T>
struct vector4:mx {
    v4<T> &data;

    ctr2(vector4, mx, v4<T>, data);
    
    inline vector4(T x, T y, T z, T w) : vector4(v4<T> { x, y, z, w })  { }
    inline vector4(v2<T> a, v2<T> b)   : vector4(a.x, a.y, b.x, b.y) { }

    inline T               dot(vector4 v) const { return data.dot(v);      }
    inline T*                     vdata() const { return data.vdata();     }
    inline real                 len_sqr() const { return data.len_sqr();   }
    inline real                     len() const { return data.len();       }
    inline vector4            normalize() const { return data.normalize(); }

    inline vector4 operator+  (vector4 v) const { return data + v.data; }
    inline vector4 operator-  (vector4 v) const { return data - v.data; }
    inline vector4 operator*  (vector4 v) const { return data * v.data; }
    inline vector4 operator/  (vector4 v) const { return data / v.data; }
    inline vector4 operator*  (T       v) const { return data * v;      }
    inline vector4 operator/  (T       v) const { return data / v;      }
    inline vector4 &operator+=(vector4 v)       { data += v; return *this; }
    inline vector4 &operator-=(vector4 v)       { data -= v; return *this; }
    inline vector4 &operator*=(vector4 v)       { data *= v; return *this; }
    inline vector4 &operator/=(vector4 v)       { data /= v; return *this; }
    inline vector4 &operator*=(T       v)       { data *= v; return *this; }
    inline vector4 &operator/=(T       v)       { data /= v; return *this; }
};

template <typename T>
struct edge2:mx {
    using e2 = e2<T>;
    using v2 = v2<T>;
    using v4 = v4<T>;

    e2 &data;

    ctr(edge2, mx, e2, data);

    edge2(v2 a, v2 b) : edge2() {
        data.a = a;
        data.b = b;
    }

    ///
    inline v2 &a() const { return data.a; }
    inline v2 &b() const { return data.b; }

    /// returns x-value of intersection of two lines
    inline T x(v2 c, v2 d) const { return data.x(c, d); }
    
    /// returns y-value of intersection of two lines
    inline T y(v2 c, v2 d) const {
        v2 &a = data.a;
        v2 &b = data.b;
        T num = (a.x*b.y  -  a.y*b.x) * (c.y-d.y) - (a.y-b.y) * (c.x*d.y - c.y*d.x);
        T den = (a.x-b.x) * (c.y-d.y) - (a.y-b.y) * (c.x-d.x);
        return num / den;
    }
    
    /// returns xy-value of intersection of two lines
    inline v2 xy(v2 c, v2 d) const { return data.xy(c, d); }

    /// returns x-value of intersection of two lines
    inline T x(edge2 e)      const { return data.x(e); }
    
    /// returns y-value of intersection of two lines
    inline T y(edge2 e)      const { return data.y(e); }

    /// returns xy-value of intersection of two lines (via edge0 and edge1)
    inline v2 xy(edge2 e)    const { return data.xy(e); }
};

template <typename T>
struct matrix44:mx {
    struct data {
        v4<T> values[4];
    } &m;

    ctr(matrix44, mx, data, m);
    
    matrix44<T> transpose() {
        matrix44<T> r;
        for (size_t j = 0; j < 4; j++) {
            v4<T> &jv = m.values[j];
            for (size_t i = 0; i < 4; i++)
                r.m.values[i][j] = jv[i];
        }
        return r;
    }
    
    matrix44<T> operator+(matrix44<T> b) {
        auto &a = *this;
        return data {
            { a.m.values[0] + b.m.values[0],
              a.m.values[1] + b.m.values[1],
              a.m.values[2] + b.m.values[2],
              a.m.values[3] + b.m.values[3] }
        };
    }
    
    matrix44<T> operator-(matrix44<T> b) {
        auto &a = *this;
        return data {
            a[0] - b[0],
            a[1] - b[1],
            a[2] - b[2],
            a[3] - b[3]
        };
    }
    
    matrix44<T> scale(v4<T> &sc) {
        matrix44<T> r;
        for     (size_t j = 0; j < 4; j++)
            for (size_t i = 0; i < 4; i++)
                r.m.values[i][j] = m.values[i][j] * sc[j];
        return r;
    }

    inline matrix44<T> operator*(matrix44<T> b) {
        matrix44<T> &a = *this;
        v4<T>      &a0 = a[0]; v4<T>     &b0 = b[0];
        v4<T>      &a1 = a[1]; v4<T>     &b1 = b[1];
        v4<T>      &a2 = a[2]; v4<T>     &b2 = b[2];
        v4<T>      &a3 = a[3]; v4<T>     &b3 = b[3];
        ///
        matrix44<T> r;
        r[0] = a0 * b0.x + a1 * b0.y + a2 * b0.z + a3 * b0.w;
        r[1] = a0 * b1.x + a1 * b1.y + a2 * b1.z + a3 * b1.w;
        r[2] = a0 * b2.x + a1 * b2.y + a2 * b2.z + a3 * b2.w;
        r[3] = a0 * b3.x + a1 * b3.y + a2 * b3.z + a3 * b3.w;
        return r;
    }
    
    inline v4<T> operator*(v4<T> b) {
        v4<T> r { 0.0, 0.0, 0.0, 0.0 };
        for     (size_t j = 0; j < 4; j++)
            for (size_t i = 0; i < 4; i++)
                r[j] += b[i] * m[i * 4 + j];
        return r;
    }

    matrix44(v4<T> v0, v4<T> v1, v4<T> v2, v4<T> v3) : matrix44() {
        size_t n_bytes = sizeof(T) * 4;
        memcpy(m.values[0].data(), v0.data(), n_bytes);
        memcpy(m.values[1].data(), v1.data(), n_bytes);
        memcpy(m.values[2].data(), v2.data(), n_bytes);
        memcpy(m.values[3].data(), v3.data(), n_bytes);
    }

    matrix44<T> rotate_arb(v3<T> v, T degs) {
        const float angle = radians(degs);
        T s = sinf(angle);
        T c = cosf(angle);
        T l = v.dot();

        /// adopt an epsilon today. save the vectors
        if (l < 1e-5) return *this;
        
        v3<T> u = v / sqrt(l);
        matrix44<T> t;
        T *tm = t.values();
        for (size_t i = 0; i < 4; i++)
            for (size_t j = 0; j < 4; j++)
                tm[i * 4 + j] = (i<3 && j<3) ? T(u[i]) * T(u[j]) : T(0.0);
        
        matrix44<T> ss = {
            {  T(0.0),   T(u[2]), -T(u[1]), T(0.0) },
            { -T(u[2]), -T(u[1]),  T(u[0]), T(0.0) },
            {  T(u[1]), -T(u[0]),  T(0.0),  T(0.0) },
            {  T(0.0),   T(0.0),   T(0.0),  T(0.0) }
        };

        matrix44<T> id = matrix44<T>::ident();
        matrix44<T> id_m_t = id - t;
        v4<T>       vc { c, c, c, c };
        matrix44<T> cc  = id_m_t.scale(vc);
        matrix44<T> rt  = t + cc + ss;
        rt[3][3]        = T(1.0);
        return *this * rt;
    }

    static matrix44<T> rotation_x(T deg) {
        const T rad = radians(deg), cos_angle = cos(rad), sin_angle = sin(rad);
        return matrix44<T> {
            data {
                v4<T> { 1.0, 0.0,       0.0,       0.0 },
                v4<T> { 0.0, cos_angle, -sin_angle, 0.0 },
                v4<T> { 0.0, sin_angle, cos_angle,  0.0 },
                v4<T> { 0.0, 0.0,       0.0,       1.0 }
            }
        };
    }

    static matrix44<T> rotation_y(T deg) {
        const T rad = radians(deg), cos_angle = cos(rad), sin_angle = sin(rad);
        return matrix44<T> {
            data {
                v4<T> { cos_angle,  0.0, sin_angle, 0.0 },
                v4<T> { 0.0,        1.0, 0.0,       0.0 },
                v4<T> { -sin_angle, 0.0, cos_angle, 0.0 },
                v4<T> { 0.0,        0.0, 0.0,       1.0 }
            }
        };
    }

    static matrix44<T> rotation_z(T deg) {
        const T rad = radians(deg), cos_angle = cos(rad), sin_angle = sin(rad);
        return matrix44<T> {
            data {
                v4<T> { cos_angle, -sin_angle, 0.0, 0.0 },
                v4<T> { sin_angle, cos_angle,  0.0, 0.0 },
                v4<T> { 0.0,       0.0,        1.0, 0.0 },
                v4<T> { 0.0,       0.0,        0.0, 1.0 }
            }
        };
    }
    
    matrix44<T> rotate_x(T deg) {
        return *this * matrix44<T>::rotation_x(deg);
    }
    
    matrix44<T> rotate_y(T deg) {
        return *this * matrix44<T>::rotation_y(deg);
    }
    matrix44<T> rotate_z(T deg) {
        return *this * matrix44<T>::rotation_z(deg);
    }

    static matrix44<T> ident() {
        return matrix44<T> {
            data {
                v4<T> { 1.0, 0.0, 0.0, 0.0 },
                v4<T> { 0.0, 1.0, 0.0, 0.0 },
                v4<T> { 0.0, 0.0, 1.0, 0.0 },
                v4<T> { 0.0, 0.0, 0.0, 1.0 }
            }
        };
    }

    /// grab vector column
    inline v4<T> &operator[](size_t y) const {
        return m.values[y];
    }

    static matrix44<T> perspective(T degrees, T aspect, const v2<T> &clip, bool flip_y) {
        const T fovy  = radians(degrees);
        const T cnear = clip.x;
        const T cfar  = clip.y;
        const T fphi  = tan(fovy / T(2.0));
        const T v00   =   T(1.0) / (aspect * fphi);
        const T v11   =  (flip_y ? -1 : 1) * T(1.0) / fphi;
        const T v22   =  (cfar) / (cnear - cfar);
        const T v23   = - T(1.0);
        const T v32   = -(cfar * cnear) / (cfar - cnear);
        ///
        return matrix44<T> {
            data {
                v4<T> { v00, 0.0, 0.0, 0.0 },
                v4<T> { 0.0, v11, 0.0, 0.0 },
                v4<T> { 0.0, 0.0, v22, v23 },
                v4<T> { 0.0, 0.0, v32, 0.0 }
            }
        };
    }

    static matrix44<T> ortho(v2<T> px, v2<T> py, v2<T> clip) {
        const T l = px.x;
        const T r = px.y;
        const T b = py.x;
        const T t = py.y;
        const T n = clip.x;
        const T f = clip.y;
        return matrix44<T> {
            data {
                v4<T> { T(2) / (r - l),  0.0, 0.0, 0.0 },
                v4<T> { 0.0,  T(2) / (t - b), 0.0, 0.0 },
                v4<T> { 0.0, 0.0, T(-2) / (f - n), 0.0 },
                v4<T> { -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 0.0 }
            }
        };
    }

    static matrix44<T> look_at(const v3<T> &e, const v3<T> &cen, const v3<T> &up) {
        const v3<T> f =   (cen - e).normalize();
        const v3<T> cp = f.cross(up);
        const v3<T> s = cp.normalize();
        const v3<T> u =  s.cross(f);
        return matrix44<T> {
            data {
                v4<T> {  s.x,       u.x,      -f.x,       0.0 },
                v4<T> {  s.y,       u.y,      -f.y,       0.0 },
                v4<T> {  s.z,       u.z,      -f.z,       0.0 },
                v4<T> { -s.dot(e), -u.dot(e),  f.dot(e),  1.0 }
            }
        };
    }

    T *values() { return (T *)m.values; }
};

template <typename T>
struct r4 {
    using v2 = v2<T>;
    T x, y, w, h;
    
    inline r4(T x = 0, T y = 0, T w = 0, T h = 0) : x(x), y(y), w(w), h(h) { }
    inline r4(v2 p0, v2 p1) : x(p0.x), y(p0.y), w(p1.x - p0.x), h(p1.y - p0.y) { }

    inline r4<T> offset(T a)              const { return { x - a, y - a, w + (a * 2), h + (a * 2) }; }
    inline v2    size()                   const { return { w, h }; }
    inline v2    xy()                     const { return { x, y }; }
    inline v2    center()                 const { return { x + w / 2.0, y + h / 2.0 }; }
    inline bool  contains(v2 p)           const { return p.x >= x && p.x <= (x + w) && p.y >= y && p.y <= (y + h); }
    inline bool  operator== (const r4 &r) const { return x == r.x && y == r.y && w == r.w && h == r.h; }
    inline bool  operator!= (const r4 &r) const { return !operator==(r); }
    inline r4<T> operator + (r4<T>  r)    const { return { x + r.x, y + r.y, w + r.w, h + r.h }; }
    inline r4<T> operator - (r4<T>  r)    const { return { x - r.x, y - r.y, w - r.w, h - r.h }; }
    inline r4<T> operator + (v2     v)    const { return { x + v.x, y + v.y, w, h }; }
    inline r4<T> operator - (v2     v)    const { return { x - v.x, y - v.y, w, h }; }
    inline r4<T> operator * (T      r)    const { return { x * r, y * r, w * r, h * r }; }
    inline r4<T> operator / (T      r)    const { return { x / r, y / r, w / r, h / r }; }
    inline       operator r4<int>   ()    const { return {    int(x),    int(y),    int(w),    int(h) }; }
    inline       operator r4<float> ()    const { return {  float(x),  float(y),  float(w),  float(h) }; }
    inline       operator r4<double>()    const { return { double(x), double(y), double(w), double(h) }; }
    inline       operator bool()          const { return w > 0 && h > 0; }
};

using m44   = matrix44 <real>;
using m44d  = matrix44 <double>;
using m44f  = matrix44 <float>;

using vec2  = vector2 <real>;
using vec2i = vector2 <i32>;
using vec2d = vector2 <r64>;
using vec2f = vector2 <r32>;

using v2r   =      v2 <real>;
using v2i   =      v2 <i32>;
using v2d   =      v2 <r64>;
using v2f   =      v2 <r32>;

using vec3  = vector3 <real>;
using vec3i = vector3 <i32>;
using vec3d = vector3 <r64>;
using vec3f = vector3 <r32>;

using v3r   =      v3 <real>;
using v3i   =      v3 <i32>;
using v3d   =      v3 <r64>;
using v3f   =      v3 <r32>;

using vec4  = vector4 <real>;
using vec4i = vector4 <i32>;
using vec4d = vector4 <r64>;
using vec4f = vector4 <r32>;

using v4r   =      v4 <real>;
using v4i   =      v4 <i32>;
using v4d   =      v4 <r64>;
using v4f   =      v4 <r32>;

using c4b   =      c4 <u8>;
using c4r   =      c4 <real>;
using c4d   =      c4 <r64>;
using c4f   =      c4 <r32>;

using r4r   =      r4 <real>;
using r4i   =      r4 <i32>;
using r4d   =      r4 <r64>;
using r4f   =      r4 <r32>;
