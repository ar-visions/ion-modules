#pragma once
#include <dx/dx.hpp>

void rgba_get_data(uint8_t *, var &);
void rgba_get_data(float   *, var &);
void rgba_get_data(double  *, var &);
void rgba_set_data(uint8_t *, var &);
void rgba_set_data(float   *, var &);
void rgba_set_data(double  *, var &);

template <typename T>
struct RGBA {
    alignas(T) T r = 0, g = 0, b = 0, a = 0;
    RGBA(std::nullptr_t n = nullptr) { }
    RGBA(real r, real g, real b, real a) {
        if constexpr (std::is_same_v<T, uint8_t>) {
            const auto sc = real(255.0);
            this->r = std::clamp(r * sc, 0.0, 255.0);
            this->g = std::clamp(g * sc, 0.0, 255.0);
            this->b = std::clamp(b * sc, 0.0, 255.0);
            this->a = std::clamp(a * sc, 0.0, 255.0);
        } else {
            this->r = r;
            this->g = g;
            this->b = b;
            this->a = a;
        }
    }
    RGBA(vec4 v) {
        if constexpr (std::is_same_v<T, uint8_t>) {
            r = v.x * 255.0;
            g = v.y * 255.0;
            b = v.z * 255.0;
            a = v.w * 255.0;
        } else {
            r = v.x;
            g = v.y;
            b = v.z;
            a = v.w;
        }
    }
    RGBA(cchar_t *pc) {
        parse(pc);
    }
    RGBA(var &d) {
        if (d == Type::Str) {
            parse(d);
        } else {
            if      constexpr (std::is_same_v<T, uint8_t>)
                rgba_get_data((uint8_t *)this, d);
            else if constexpr (std::is_same_v<T, float>)
                rgba_get_data((float *)this,   d);
            else if constexpr (std::is_same_v<T, double>)
                rgba_get_data((double *)this,  d);
        }
    }
    operator str() {
        char str[64];
        uint8_t arr[4];
        double sc = 255.0;
        if constexpr (std::is_same_v<T, uint8_t>)
            sc = 1;
        arr[0] = uint8_t(std::round(r * sc));
        arr[1] = uint8_t(std::round(g * sc));
        arr[2] = uint8_t(std::round(b * sc));
        arr[3] = uint8_t(std::round(a * sc));
        for (size_t i = 0, len = sizeof(arr) - (arr[3] == 255); i < len; i++)
            sprintf(&str[i * 2], "%02x", arr[i]);
        return (cchar_t *)str;
    }
    operator var() {
        var d;
        if      constexpr (std::is_same_v<T, uint8_t>)
            rgba_set_data((uint8_t *)this, d);
        else if constexpr (std::is_same_v<T, float>)
            rgba_set_data((float *)this,   d);
        else if constexpr (std::is_same_v<T, double>)
            rgba_set_data((double *)this,  d);
        return d;
    }
    void parse(str s) {
        cchar_t *pc = s.cstr();
        str      h = pc;
        size_t  sz = h.size();
        int32_t ir = 0, ig = 0,
                ib = 0, ia = 255;
        if (sz && h[0] == '#') {
            auto nib = [&](char const n) -> int32_t const {
                return (n >= '0' && n <= '9') ?       (n - '0')  :
                       (n >= 'a' && n <= 'f') ? (10 + (n - 'a')) :
                       (n >= 'A' && n <= 'F') ? (10 + (n - 'A')) : 0;
            };
            switch (sz) {
                case 5:
                    ia  = nib(h[4]) << 4 | nib(h[4]);
                    [[fallthrough]];
                case 4:
                    ir  = nib(h[1]) << 4 | nib(h[1]);
                    ig  = nib(h[2]) << 4 | nib(h[2]);
                    ib  = nib(h[3]) << 4 | nib(h[3]);
                    break;
                case 9:
                    ia  = nib(h[7]) << 4 | nib(h[8]);
                    [[fallthrough]];
                case 7:
                    ir  = nib(h[1]) << 4 | nib(h[2]);
                    ig  = nib(h[3]) << 4 | nib(h[4]);
                    ib  = nib(h[5]) << 4 | nib(h[6]);
                    break;
            }
        }
        if constexpr (sizeof(T) == 1) {
            r = T(ir);
            g = T(ig);
            b = T(ib);
            a = T(ia);
        } else {
            r = T(std::round(T(ir) / T(255.0)));
            g = T(std::round(T(ig) / T(255.0)));
            b = T(std::round(T(ib) / T(255.0)));
            a = T(std::round(T(ia) / T(255.0)));
        }
    }
    real scale() const {
        return sizeof(T) == 1 ? 255.0 : 1.0;
    }
    inline RGBA<T> operator+(RGBA<T> b) {
        const real sc = scale();
        return RGBA<T> {
            real(this->r + b.r) / sc,
            real(this->g + b.g) / sc,
            real(this->b + b.b) / sc,
            real(this->a + b.a) / sc
        };
    }
    inline RGBA<T> operator*(real f) {
        real m = f / scale();
        return RGBA<T> { r * m, g * m, b * m, a * m };
    }
    operator vec4() {
        const real sc = scale();
        return vec4 { r / sc, g / sc, b / sc, a / sc };
    }
    std::vector<double> doubles() {
        const real sc = scale();
        return { r / sc, g / sc, b / sc, a / sc };
    }
    bool operator!() { return a <= 0; }
    operator bool()  { return a >  0; }
};

typedef RGBA<uint8_t> rgba;

template <typename T>
struct Weighted {
    real p; /// pos
    real w; /// weight
    T    v; /// value
};

