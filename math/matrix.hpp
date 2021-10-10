#pragma once
#include <data/data.hpp>
#include <data/vec.hpp>

template <typename T>
T radians(T degrees) {
    return degrees / T(180.0) * (T(2.0) * T(M_PI));
}

template <typename T>
T degrees(T radians) {
    return radians / (T(2.0) * T(M_PI)) * T(180.0);
}

template <typename T>
struct Matrix44:Vector<T> {
    T e[16];
    
    Matrix44() {
        memset(&e[0], 0, sizeof(T) * 16);
    }
    
    Matrix44(nullptr_t n) : Matrix44() {
        e[0] = std::numeric_limits<double>::quiet_NaN();
    }
    
    Matrix44(std::vector<T> values) : Matrix44() {
        const size_t sz = values.size();
        assert(sz == 16);
        for (size_t i = 0; i < sz; i++)
            e[i] = values[i];
    }
    
    void copy(Matrix44<T> &ref) {
        std::memcpy(&e[0], &ref.e[0], sizeof(T) * 16);
    }
    
    Matrix44(Matrix44<T> &ref) {
        if (this != &ref)
            copy(ref);
    }

    Matrix44(const T *data) { memcopy(e, data, sizeof(e)); }
    
    T *data() { return e; }
    
    static Matrix44<T> identity() {
        auto m = Matrix44<T>();
        m[0]   = 1;
        m[5]   = 1;
        m[10]  = 1;
        m[15]  = 1;
        return m;
    }
    
    static Matrix44<T> scale(vec3 s) {
        auto r = Matrix44<T>::identity();
        r[0]   = s.x;
        r[5]   = s.y;
        r[10]  = s.z;
        return r;
    }

    static Matrix44<T> translation(vec3 t) {
        auto r = Matrix44<T>::identity();
        r[3]   = t.x;
        r[7]   = t.y;
        r[11]  = t.z;
        return r;
    }

    static Matrix44<T> rotation_x(T deg) {
        T rads =  radians(deg);
        auto r =  Matrix44<T>::identity();
        r[5]   =  std::cos(-rads);
        r[6]   = -std::sin(-rads);
        r[9]   =  std::sin(-rads);
        r[10]  =  std::cos(-rads);
        return r;
    }

    static Matrix44<T> rotation_y(T deg) {
        T rads =  radians(deg);
        auto r =  Matrix44<T>::identity();
        r[0]   =  std::cos(-rads);
        r[2]   =  std::sin(-rads);
        r[8]   = -std::sin(-rads);
        r[10]  =  std::cos(-rads);
        return r;
    }

    static Matrix44<T> rotation_z(T deg) {
        T rads =  radians(deg);
        auto r =  Matrix44<T>::identity();
        r[0]   =  std::cos(-rads);
        r[1]   = -std::sin(-rads);
        r[4]   =  std::sin(-rads);
        r[5]   =  std::cos(-rads);
        return r;
    }

    static Matrix44<T> view(vec3 forward, vec3 up, vec3 right, vec3 position) {
        auto o =  Matrix44<T>::identity();
        o[0]   =  right.x;
        o[1]   =  right.y;
        o[2]   =  right.z;
        o[4]   =  up.x;
        o[5]   =  up.y;
        o[6]   =  up.z;
        o[8]   =  forward.x;
        o[9]   =  forward.y;
        o[10]  =  forward.z;
        auto m =  Matrix44<T>::identity();
        m[3]   = -position.x;
        m[7]   = -position.y;
        m[11]  = -position.z;
        Matrix44<T> r = o * m;
        return r;
    }

    static Matrix44<T> perspective(T width, T height, T fov, T near, T far) {
        T fov_rads = fov * M_PI / 180.0 / 2.0;
        auto r = Matrix44<T>::identity();
        r[0]   = (T(1.0) / std::tan(fov_rads)) / T(width / height);
        r[5]   =  T(1.0) / std::tan(fov_rads);
        r[10]  =  T(far + near) / T(near - far);
        r[11]  =  T(2.0 * far * near) / T(near - far);
        r[14]  =  T(-1.0);
        return r;
    }

    static Matrix44<T> ortho(T left, T right, T top, T bottom, T far, T near) {
        auto r = Matrix44<T>::identity();
        r[0]   = 2.0f / (right - left);
        r[3]   = -((right + left) / (right - left));
        r[5]   = 2.0f / (top - bottom);
        r[7]   = -((top + bottom) / (top - bottom));
        r[10]  = 2.0f / (far - near);
        r[11]  = -((far + near) / (far - near));
        return r;
    }
    
    T &operator[](size_t i) {
        return e[i];
    }

    Matrix44<T>& operator+=(Matrix44<T> &rhs) {
        for (size_t i = 0; i < 16; i++)
            (*this)[i] += rhs[i];
        return *this;
    }

    Matrix44<T>& operator-=(Matrix44<T> &rhs) {
        for (size_t i = 0; i < 16; i++)
            (*this)[i] -= rhs[i];
        return *this;
    }

    Matrix44<T>& operator*=(Matrix44& rhs) {
        Matrix44<T> &s = (*this);
        s[0]  =  s[0] * rhs[0] +  s[1] * rhs[4] +  s[2] *  rhs[8]  + s[3]  * rhs[12];
        s[1]  =  s[0] * rhs[1] +  s[1] * rhs[5] +  s[2] *  rhs[9]  + s[3]  * rhs[13];
        s[2]  =  s[0] * rhs[2] +  s[1] * rhs[6] +  s[2] * rhs[10]  + s[3]  * rhs[14];
        s[3]  =  s[0] * rhs[3] +  s[1] * rhs[7] +  s[2] * rhs[11]  + s[3]  * rhs[15];

        s[4]  =  s[4] * rhs[0] +  s[5] * rhs[4] +  s[6] *  rhs[8]  + s[7]  * rhs[12];
        s[5]  =  s[4] * rhs[1] +  s[5] * rhs[5] +  s[6] *  rhs[9]  + s[7]  * rhs[13];
        s[6]  =  s[4] * rhs[2] +  s[5] * rhs[6] +  s[6] * rhs[10]  + s[7]  * rhs[14];
        s[7]  =  s[4] * rhs[3] +  s[5] * rhs[7] +  s[6] * rhs[11]  + s[7]  * rhs[15];

        s[8]  =  s[8] * rhs[0] +  s[9] * rhs[4] + s[10] *  rhs[8]  + s[11] * rhs[12];
        s[9]  =  s[8] * rhs[1] +  s[9] * rhs[5] + s[10] *  rhs[9]  + s[11] * rhs[13];
        s[10] =  s[8] * rhs[2] +  s[9] * rhs[6] + s[10] * rhs[10]  + s[11] * rhs[14];
        s[11] =  s[8] * rhs[3] +  s[9] * rhs[7] + s[10] * rhs[11]  + s[11] * rhs[15];

        s[12] = s[12] * rhs[0] + s[13] * rhs[4] + s[14] *  rhs[8]  + s[15] * rhs[12];
        s[13] = s[12] * rhs[1] + s[13] * rhs[5] + s[14] *  rhs[9]  + s[15] * rhs[13];
        s[14] = s[12] * rhs[2] + s[13] * rhs[6] + s[14] * rhs[10]  + s[15] * rhs[14];
        s[15] = s[12] * rhs[3] + s[13] * rhs[7] + s[14] * rhs[11]  + s[15] * rhs[15];
        return *this;
    }

    inline Matrix44<T> &operator*=(T& rhs) {
        for(int i = 0; i < 16; i++)
            e[i] *= rhs;
        return *this;
    }

    inline Matrix44<T> operator+(Matrix44<T> rhs) {
        auto r = *this;
        r += rhs;
        return r;
    }

    inline Matrix44<T> operator-(Matrix44<T> rhs) {
        auto r = *this;
        r -= rhs;
        return r;
    }

    inline Matrix44<T> operator*(Matrix44<T> rhs) {
        auto r = *this;
        r *= rhs;
        return r;
    }

    inline Matrix44<T> operator*(T& rhs) {
        auto r = *this;
        r *= rhs;
        return r;
    }
};

typedef Matrix44<double> m44;
typedef Matrix44<float>  m44f;

