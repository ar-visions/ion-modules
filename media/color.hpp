#pragma once
#include <data/data.hpp>

void rgba_get_data(uint8_t *, var &);
void rgba_get_data(float   *, var &);
void rgba_get_data(double  *, var &);
void rgba_set_data(uint8_t *, var &);
void rgba_set_data(float   *, var &);
void rgba_set_data(double  *, var &);

template <typename T>
struct RGBA {
    alignas(T) T r = 0, g = 0, b = 0, a = 0;
    RGBA(nullptr_t n = nullptr) { }
    RGBA(double r, double g, double b, double a) {
        double sc = 1;
        if constexpr (std::is_same_v<T, uint8_t>)
            sc = 1;
        this->r = r * sc;
        this->g = g * sc;
        this->b = b * sc;
        this->a = a * sc;
    }
    RGBA(const char *pc) {
        parse(pc);
    }
    RGBA(var &d) {
        if (d == var::Str) {
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
        return (const char *)str;
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
        const char *pc = s.cstr();
        str      h = pc;
        size_t  sz = h.length();
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
        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
            r = ir;
            g = ig;
            b = ib;
            a = ia;
        } else {
            r = ir / T(255.0);
            g = ig / T(255.0);
            b = ib / T(255.0);
            a = ia / T(255.0);
        }
    }
    std::vector<double> doubles() {
        if constexpr (std::is_same_v<T, uint8_t>)
            return { r / 255.0, g / 255.0, b / 255.0, a / 255.0 };
        assert(false);
    }
    bool operator!() { return a <= 0; }
    operator bool()  { return a >  0; }
};

typedef RGBA<uint8_t> rgba;
