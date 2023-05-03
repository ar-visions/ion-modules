#pragma once

template <typename T>
struct color:vec<T, 4> {
    using  base = vec<T,4>;
    using  data = c4 <T>;

    data &m;
    
    ctr(color, base, data, m);

    inline color(T *d, size_t sz) : color(mx::view<T>(d, 4, sz)) { }

    template <typename V>
    color(V r, V g, V b, V a):color() {
        if constexpr (sizeof(V) == 1) {
            m.r = T(math::round(r * 255.0)); m.g = T(math::round(g * 255.0));
            m.b = T(math::round(b * 255.0)); m.a = T(math::round(a * 255.0));
        } else {
            m.r = T(r); m.g = T(g);
            m.b = T(b); m.a = T(a);
        }
    }

    color(v4<real> &v):color() {
        m.r = v.x; m.g = v.y;
        m.b = v.z; m.a = v.w;
    }

    color(cstr pc):color() {
        str    h  = pc;
        size_t sz = h.len();
        i32    ir = 0, ig = 0,
               ib = 0, ia = 255;
        if (sz && h[0] == '#') {
            auto nib = [&](char const n) -> i32 const {
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
            m.r = T(ir);
            m.g = T(ig);
            m.b = T(ib);
            m.a = T(ia);
        } else {
            m.r = T(math::round(T(ir) / T(255.0)));
            m.g = T(math::round(T(ig) / T(255.0)));
            m.b = T(math::round(T(ib) / T(255.0)));
            m.a = T(math::round(T(ia) / T(255.0)));
        }
    }

    color(str s) : color(s.data) { }

    template <typename B>
    operator v4<B>() const {
        if constexpr (identical<T, B>())
            return v4<B>(m);
        else {
            if constexpr (sizeof(B) == 1 && sizeof(T) > 1) {
                return v4<B> { m.r * 255.0, m.g * 255.0, m.b * 255.0, m.a * 255.0 };
            } else if constexpr (sizeof(T) == 1 && sizeof(B) > 1) {
                return v4<B> { B(m.r / 255.0), B(m.g / 255.0), B(m.b / 255.0), B(m.a / 255.0) };
            } else {
                return v4<B> { B(m.r), B(m.g), B(m.b), B(m.a) };
            }
        }
    }
    
    /// #hexadecimal formatted str
    operator str() {
        char    res[64];
        u8      arr[4];
        ///
        real sc;
        ///
        if constexpr (identical<T, uint8_t>())
            sc = 1.0;
        else
            sc = 255.0;
        ///
        arr[0] = uint8_t(math::round(m.r * sc));
        arr[1] = uint8_t(math::round(m.g * sc));
        arr[2] = uint8_t(math::round(m.b * sc));
        arr[3] = uint8_t(math::round(m.a * sc));
        ///
        res[0]  = '#';
        ///
        for (size_t i = 0, len = sizeof(arr) - (arr[3] == 255); i < len; i++) {
            size_t index = 1 + i * 2;
            snprintf(&res[index], sizeof(res) - index, "%02x", arr[i]); /// secure.
        }
        ///
        return (symbol)res;
    }

    real scale() const { return sizeof(T) == 1 ? 255.0 : 1.0; }

    inline color<T> operator+(color<T> b) {
        const real sc = scale();
        return color<T> {
            real(m.r + b.r) / sc,
            real(m.g + b.g) / sc,
            real(m.b + b.b) / sc,
            real(m.a + b.a) / sc
        };
    }

    inline color<T> operator*(real f) {
        real sc = f / scale();
        return color<T> { m.r * sc, m.g * sc, m.b * sc, m.a * sc };
    }

    bool operator==(color<T> p) { return m.r == p.m.r && m.g == p.m.g && m.b == p.m.b && m.a == p.m.a; }
    bool operator!=(color<T> p) { return !operator==(p); };
    bool operator!()            { return m.a <= 0; }
    operator bool()             { return m.a >  0; }

};

using rgba = color<u8>;

struct image:array<rgba::data> {
    image(memory *m)       : array<rgba::data>(m)  { }
    image(size   sz)       : array<rgba::data>(sz) { }
    image(null_t n = null) : image(size { 1, 1 })  { }
    image(path p);
    image(size sz, rgba::data *px, int scanline = 0);
    bool save(path p) const;
    rgba::data *pixels() const { return elements; }
    size_t       width() const { return (*mem->shape)[1]; }
    size_t      height() const { return (*mem->shape)[0]; }
    size_t      stride() const { return (*mem->shape)[1]; }
    r4i           rect() const { return { 0, 0, int(width()), int(height()) }; }
    rgba::data &operator[](size pos) const {
        size_t index = mem->shape->index_value(pos);
        return elements[index];
    }
};
