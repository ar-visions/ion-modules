import <cmath>
import dx:vec;

export module dx:m44;
export {



/*
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi
*/

// implemented with reference guide linmath; always used linmath prior in orion & hyper race projects. great people.

template <typename T>
T radians(T degrees) {
    return degrees / T(180.0) * (T(2.0) * T(M_PI));
}

template <typename T>
T degrees(T radians) {
    return radians / (T(2.0) * T(M_PI)) * T(180.0);
}

template <typename T>
struct Matrix44 {
    T m[16] = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    ///
    T &  operator()(size_t i)           { return m[i]; }
    T &  operator()(size_t i, size_t j) { return m[i * 4 + j]; }
    bool operator==(Matrix44<T> &b)     { return memcmp(m, b.m, sizeof(T) * 16); }
    bool operator!=(Matrix44<T> &b)     { return !operator==(b); }
    
    /// no boolean operator here
    Matrix44()                 { }
    Matrix44(std::nullptr_t n) { }
    Matrix44(Vec4<T> v0, Vec4<T> v1, Vec4<T> v2, Vec4<T> v3) {
        auto &a = *this;
        a[0] = v0;
        a[1] = v1;
        a[2] = v2;
        a[3] = v3;
    }
    
              Matrix44(      Matrix44<T> &ref) { copy(ref);                    }
              Matrix44(const Matrix44<T> &ref) { copy((Matrix44<T> &)ref);     }
              Matrix44(const T          *data) { memcpy(m, data,  sizeof(m));  }
    void          copy(Matrix44<T>       &ref) { memcpy(m, ref.m, sizeof(m));  }
         T       *data()                 const { &m[0];                        }
    Vec4<T>  &operator[] (size_t i)      const { return *(Vec4<T> *)&m[i * 4]; } /// it'll work.. it'll work. OW. chewy!
    
    static Matrix44<T> translation(Vec3<T> v) {
        Matrix44<T> r;
        r[3] = Vec4<T> { v.x, v.y, v.z, 1.0 };
        return r;
    }
    
    static Matrix44<T> translation(Vec2<T> v) {
        Matrix44<T> r;
        r[3] = Vec4<T> { v.x, v.y, 0.0, 1.0 };
        return r;
    }
    
    static Matrix44<T> rotation_x(T deg) {
        T       angle = radians(deg);
        T           s = std::sin(angle);
        T           c = std::cos(angle);
        return Matrix44<T> {
            { 1.0, 0.0, 0.0, 0.0 },
            { 0.0,   c,   s, 0.0 },
            { 0.0,  -s,   c, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        };
    }
    
    static Matrix44<T> rotation_y(T deg) {
        T       angle = radians(deg);
        T           s = std::sin(angle);
        T           c = std::cos(angle);
        return Matrix44<T> {
            {   c, 0.0,  -s, 0.0},
            { 0.0, 1.0, 0.0, 0.0},
            {   s, 0.0,   c, 0.0},
            { 0.0, 0.0, 0.0, 1.0}
        };
    }
    
    static Matrix44<T> rotation_z(T deg) {
        T      angle = radians(deg);
        T          s = std::sin(angle);
        T          c = std::cos(angle);
        return Matrix44<T> {
            {   c,   s, 0.0, 0.0},
            {  -s,   c, 0.0, 0.0},
            { 0.0, 0.0, 1.0, 0.0},
            { 0.0, 0.0, 0.0, 1.0}
        };
    }
    
    Matrix44 &operator=(const Matrix44<T> &ref) {
        if (this != &ref)
            copy((Matrix44<T> &)ref);
        return *this;
    }
    
    Matrix44<T> transpose() {
        Matrix44<T> r;
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                r.m[i * 4 + j] = m[j * 4 + i];
        return r;
    }
    
    Matrix44<T> operator+(Matrix44<T> b) {
        auto &a = *this;
        return { a[0] + b[0], a[1] + b[1],
                 a[2] + b[2], a[3] + b[3] };
    }
    
    Matrix44<T> operator-(Matrix44<T> b) {
        auto &a = *this;
        return { a[0] - b[0], a[1] - b[1],
                 a[2] - b[2], a[3] - b[3] };
    }
    
    Matrix44<T> scale(Vec4<T> sc) {
        Matrix44<T> r;
        for     (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                r.m[i * 4 + j] = m[i * 4 + j] * sc[j];
        return r;
    }

    inline Matrix44<T> operator*(Matrix44<T> b) {
        Matrix44<T> &a  = *this;
        Vec4<T>      a0 = a[0]; Vec4<T>      b0 = b[0];
        Vec4<T>      a1 = a[1]; Vec4<T>      b1 = b[1];
        Vec4<T>      a2 = a[2]; Vec4<T>      b2 = b[2];
        Vec4<T>      a3 = a[3]; Vec4<T>      b3 = b[3];
        ///
        Matrix44<T> r;
        r[0] = a0 * b0.x + a1 * b0.y + a2 * b0.z + a3 * b0.w;
        r[1] = a0 * b1.x + a1 * b1.y + a2 * b1.z + a3 * b1.w;
        r[2] = a0 * b2.x + a1 * b2.y + a2 * b2.z + a3 * b2.w;
        r[3] = a0 * b3.x + a1 * b3.y + a2 * b3.z + a3 * b3.w;
        return r;
    }
    
    inline Vec4<T> operator*(Vec4<T> b) {
        Vec4<T> r = Vec4<T> { 0.0, 0.0, 0.0, 0.0 };
        for     (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                r[j] += b[i] * m[i * 4 + j];
        return r;
    }
    
    static Matrix44<T> identity()     { return Matrix44<T>(); }
    ///
    Matrix44<T>     scale(Vec2<T> sc) { return scale(Vec4<T> {sc.x, sc.y, T(1.0), T(1.0)}); }
    Matrix44<T> translate(T x, T y, T z = T(0.0)) {
        Matrix44<T> r = *this;
        Vec4<T>     v = r[0] * x + r[1] * y + r[2] * z;
        r[3].x += v.x;
        r[3].y += v.y;
        r[3].z += v.z;
        return r;
    }
    Matrix44<T> translate(Vec2<T> v) { return translate(v.x, v.y, T(0.0)); }
    Matrix44<T> translate(Vec3<T> v) { return translate(v.x, v.y, v.z);    }
    
    Matrix44<T> rotate_arb(Vec3<T> v, double degs) {
        const float angle = radians(degs);
        T s = sinf(angle), c = cosf(angle);
        T l = v.len_sqr();
        if (l < 1e-5) return *this;
        
        Vec3<T> u = v / sqrt(l);
        Matrix44<T> t;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                t[i * 4 + j] = (i<3 && j<3) ? u[i] * u[j] : T(0.0);
        
        Matrix44<T> ss = {
            {   0.0,  u[2], -u[1], 0.0 },
            { -u[2], -u[1],  u[0], 0.0 },
            {  u[1], -u[0],   0.0, 0.0 },
            {   0.0,   0.0,   0.0, 0.0 }
        };
        Matrix44<T> id;
        Matrix44<T> cc = (id - t).scale(c);
        Matrix44<T> rt = t + cc + ss;
        rt[3][3] = T(1.0);
        return *this * rt;
    }
    
    Matrix44<T> rotate_x(double deg) {
        return *this * Matrix44<T>::rotation_x(deg);
    }
    
    Matrix44<T> rotate_y(double deg) {
        return *this * Matrix44<T>::rotation_y(deg);
    }
    Matrix44<T> rotate_z(double deg) { return (*this) * Matrix44<T>::rotation_z(deg); }
    
    Matrix44<T> invert(double deg) {
        Matrix44<T> r;
        auto &m = *this;
        float s[6], c[6];
        
        s[0] = m(0,0) * m(1,1) - m(1,0) * m(0,1);
        s[1] = m(0,0) * m(1,2) - m(1,0) * m(0,2);
        s[2] = m(0,0) * m(1,3) - m(1,0) * m(0,3);
        s[3] = m(0,1) * m(1,2) - m(1,1) * m(0,2);
        s[4] = m(0,1) * m(1,3) - m(1,1) * m(0,3);
        s[5] = m(0,2) * m(1,3) - m(1,2) * m(0,3);
        c[0] = m(2,0) * m(3,1) - m(3,0) * m(2,1);
        c[1] = m(2,0) * m(3,2) - m(3,0) * m(2,2);
        c[2] = m(2,0) * m(3,3) - m(3,0) * m(2,3);
        c[3] = m(2,1) * m(3,2) - m(3,1) * m(2,2);
        c[4] = m(2,1) * m(3,3) - m(3,1) * m(2,3);
        c[5] = m(2,2) * m(3,3) - m(3,2) * m(2,3);
        T  w = ( s[0]*c[5]-s[1]*c[4]+s[2]*c[3]+s[3]*c[2]-s[4]*c[1]+s[5]*c[0] ); assert(!std::isnan(w));
        T  i = 1.0f / w;
        
        return Matrix44<T> {
            Vec4<T> { ( m(1,1) * c[5] - m(1,2) * c[4] + m(1,3) * c[3]),
                      (-m(0,1) * c[5] + m(0,2) * c[4] - m(0,3) * c[3]),
                      ( m(3,1) * s[5] - m(3,2) * s[4] + m(3,3) * s[3]),
                      (-m(2,1) * s[5] + m(2,2) * s[4] - m(2,3) * s[3]) } * i,
            Vec4<T> { (-m(1,0) * c[5] + m(1,2) * c[2] - m(1,3) * c[1]),
                      ( m(0,0) * c[5] - m(0,2) * c[2] + m(0,3) * c[1]),
                      (-m(3,0) * s[5] + m(3,2) * s[2] - m(3,3) * s[1]),
                      ( m(2,0) * s[5] - m(2,2) * s[2] + m(2,3) * s[1]) } * i,
            Vec4<T> { ( m(1,0) * c[4] - m(1,1) * c[2] + m(1,3) * c[0]),
                      (-m(0,0) * c[4] + m(0,1) * c[2] - m(0,3) * c[0]),
                      ( m(3,0) * s[4] - m(3,1) * s[2] + m(3,3) * s[0]),
                      (-m(2,0) * s[4] + m(2,1) * s[2] - m(2,3) * s[0]) } * i,
            Vec4<T> { (-m(1,0) * c[3] + m(1,1) * c[1] - m(1,2) * c[0]),
                      ( m(0,0) * c[3] - m(0,1) * c[1] + m(0,2) * c[0]),
                      (-m(3,0) * s[3] + m(3,1) * s[1] - m(3,2) * s[0]),
                      ( m(2,0) * s[3] - m(2,1) * s[1] + m(2,2) * s[0]) } * i
        };
        return r;
    }
    
    static Matrix44<T> frustum(Rect<T> shape, Vec2<T> clip) {
        const T near = clip[0], far = clip[1];
        return {
            { 2.0 * near / shape.w, 0.0, 0.0, 0.0 },
            { 0.0, 2.0 * near / shape.h, 0.0, 0.0 },
            { (shape.x + shape.x + shape.w) / (shape.w)
              (shape.y + shape.y + shape.h) / (shape.b),
             -(far + near) / (far - near), -1.0 },
            { -2.0 * (far * near) / (far - near), 0.0, 0.0, 0.0 }
        };
    }
    
    
    static Matrix44<T> ortho(T left, T right, T bottom, T top, Vec2<T> clip) {
        const T near  = clip[0];
        const T far   = clip[1];
        auto   m      = Matrix44<T>::identity();
               m[0].x =              T(2) / (right - left  );
               m[1].y =              T(2) / (top   - bottom);
               m[2].z =             T(-2) / (far   - near  );
               m[3].x = -(right + left  ) / (right - left  );
               m[3].y = -(top   + bottom) / (top   - bottom);
               m[3].z = -(far   + near  ) / (far   - near  );
        return m;
    }
    
    static Matrix44<T> perspective(T deg, T aspect, Vec2<T> clip) {
        const T fovy = radians(deg);
        const T near = clip[0];
        const T far  = clip[1];
        const T phi  = tan(fovy / T(2.0));
        const T v00  =    T(1.0) / (aspect * phi);
        const T v11  =    T(1.0) / (phi);
        const T v22  = - (far + near) / (far - near);
        const T v23  = -  T(1.0);
        const T v32  = - (T(2.0) * far * near) / (far - near);
        return Matrix44<T> {
            { v00, 0.0, 0.0, 0.0 },
            { 0.0, v11, 0.0, 0.0 },
            { 0.0, 0.0, v22, v23 },
            { 0.0, 0.0, v32, 0.0 }
        };
    }
    
    static Matrix44<T> look_at(vec3 e, vec3 cen, vec3 up) {
        Vec3<T> f = (cen - e).normalize();
        Vec3<T> s = f.cross(up);
        Vec3<T> t = s.cross(f);
        Vec3<T> n = e * -1.0;
        return Matrix44<T> {
            {  s.x,  t.x, -f.x, 0.0 },
            {  s.y,  t.y, -f.y, 0.0 },
            {  s.z,  t.z, -f.z, 0.0 },
            { -e.x, -e.y, -e.z, 1.0 }
        };
    }
};

typedef Matrix44<double> m44;
typedef Matrix44<float>  m44f;
///
}
