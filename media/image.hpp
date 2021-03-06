#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>
#include <dx/m44.hpp>
#include <media/color.hpp>

struct Image {
    enum Format {
        Gray,
        Rgba
    };
    var pixels; /// our only member. so proud. [how many members does var have?] ... so proud.

    Image(std::nullptr_t n = null);
    Image(path_t, Format);
    Image(string path);
    Image(var &);
    Image(vec2i sz, Format f);
    Image(vec2i sz, rgba clr);
   ~Image();
    Image      crop(recti);
    operator    var();
    void       save(path_t);
    size_t   stride();
    vec2       size();
    bool  operator!();
    operator   bool();
    operator  recti();
    operator  vec2i();
    Image transform(m44 m, vec2i vp = {});
    Image  resample(vec2i sz);
    Format   format();
    
    static inline vec4 cubic(real v) {
        vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - vec4(v);
        vec4 s = n * n * n;
        real x = s.x;
        real y = s.y - 4.0 * s.x;
        real z = s.z - 4.0 * s.y + 6.0 * s.x;
        real w = 6.0 - x - y - z;
        return vec4(x, y, z, w);
    }
    
    template <typename T>
    static inline T bicubic_generic(vec2 uv, vec2 &tz, std::function<T(vec4 &, vec2 &)> fn) {
        vec2 itz     = vec2(1.0) / tz;
        uv           = uv * tz - vec2(0.5);
        vec2 fxy     = fract(uv);
        uv          -= fxy;
        vec4 xcubic  = cubic(fxy.x);
        vec4 ycubic  = cubic(fxy.y);
        vec4  c      = xxyy(uv) + xyxy(vec2(0.0, +2.0));
        vec4  s      = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w,
                            ycubic.x + ycubic.y, ycubic.z + ycubic.w);
        vec4 offset  = c + vec4 (xcubic.yw(), ycubic.yw()) / s;
        vec4 so      = offset * xxyy(itz);
        vec2 sv      = vec2(s.x / (s.x + s.y),
                            s.z / (s.z + s.w));
        return fn(so, sv);
    }
    
    vec4 bicubic_4(vec2 uv, vec2 &tz) {
        return bicubic_generic<vec4>(uv, tz, [&](vec4 &so, vec2 &s) {
            return mix(mix(bilinear_4(so.yw(), tz),
                           bilinear_4(so.xw(), tz), s.x),
                       mix(bilinear_4(so.yz(), tz),
                           bilinear_4(so.xz(), tz), s.x), s.y);
        });
    }
    
    real bicubic(vec2 uv, vec2 &tz) {
        return bicubic_generic<real>(uv, tz,
            [&](vec4 &so, vec2 &s) {
                real s0 = bilinear(so.xz(), tz);
                real s1 = bilinear(so.yz(), tz);
                real s2 = bilinear(so.xw(), tz);
                real s3 = bilinear(so.yw(), tz);
                return mix(mix(s3, s2, s.x),
                           mix(s1, s0, s.x), s.y);
            });
    }
    
    vec4 bilinear_4(vec2 uv, vec2 &tz) {
        vec2     px = vec2(1.0) / tz;
        vec2    pos = (uv - px / 2.0) * tz;
        vec2i    tx = pos.floor();
        vec2  blend = (pos - pos.floor()).abs();
        return  vec4(pixel<rgba>(tx.x + 0, tx.y + 0)) * (1.0 - blend.x) * (1.0 - blend.y) +
                vec4(pixel<rgba>(tx.x + 1, tx.y + 0)) * (      blend.x) * (1.0 - blend.y) +
                vec4(pixel<rgba>(tx.x + 1, tx.y + 1)) * (      blend.x) * (      blend.y) +
                vec4(pixel<rgba>(tx.x + 0, tx.y + 1)) * (1.0 - blend.x) * (      blend.y);
    }
    
    real bilinear(vec2 uv, vec2 &tz) {
        vec2     px = vec2(1.0) / tz;
        vec2    pos = (uv - px / 2.0) * tz;
        vec2i    tx = pos.floor();
        vec2  blend = (pos - pos.floor()).abs();
        return  pixel<uint8_t>(tx.x + 0, tx.y + 0) * (1.0 - blend.x) * (1.0 - blend.y) +
                pixel<uint8_t>(tx.x + 1, tx.y + 0) * (      blend.x) * (1.0 - blend.y) +
                pixel<uint8_t>(tx.x + 1, tx.y + 1) * (      blend.x) * (      blend.y) +
                pixel<uint8_t>(tx.x + 0, tx.y + 1) * (1.0 - blend.x) * (      blend.y);
    }
    
    /// this can be cleaner but i am just finishing a refactor here with shape.
    template <typename T>
    inline T &pixel(int x, int y) {
        auto   &sh   = pixels.sh;
        int     sh_w = int(sh[1]);
        int     sh_h = int(sh[0]);
        assert(sh.dims() == 3 && sh_h && sh_w);
        int       xp = (x < 0) ? 0 : ((x >= sh_w) ? (sh_w - 1) : x);
        int       yp = (y < 0) ? 0 : ((y >= sh_h) ? (sh_h - 1) : y);
        vec2i   orig = pixels.count("crop") ? vec2i(pixels["crop"]) : vec2i { 0, 0 };
        T      *data = pixels.data<T>();
        return data[(orig.y + yp) * sh_w + (orig.x + xp)];
    }
    
    inline Shape<Major> shape() {
        return pixels.shape();
    }
    
    inline void filter(std::function<rgba(rgba &, int, int)> fn) {
        assert(pixels.c == Type::ui32 || pixels.c == Type::i32);
        auto    sh   = pixels.shape();
        int     sh_d = int(sh[2]);
        int     sh_w = int(sh[1]);
        int     sh_h = int(sh[0]);
        assert( sh_d == 4 );
        recti crop = pixels.count("crop") ? recti(pixels["crop"]) : recti { 0, 0, sh_w, sh_h };
        rgba    *o = (rgba *)pixels.data<uint32_t>() + crop.y * sh_w + crop.x;
        for (int y = 0; y < crop.h; y++) {
            rgba *p = &o[sh_w * y];
            for (int x = 0; x < crop.w; x++, p++)
                *p = fn(*p, x, y);
        }
    }
    
    inline void filter(std::function<uint8_t(uint8_t &, int, int)> fn) {
        assert(pixels.c == Type::ui8 || pixels.c == Type::i8);
        auto    sh   = pixels.shape();
        int     sh_d = int(sh[2]);
        int     sh_w = int(sh[1]);
        int     sh_h = int(sh[0]);
        assert( sh_d == 1 );
        recti   crop = pixels.count("crop") ? recti(pixels["crop"]) : recti { 0, 0, sh_w, sh_h };
        uint8_t   *o = &pixels.data<uint8_t>()[crop.y * sh_w + crop.x];
        for (int   y = 0; y < crop.h; y++) {
            uint8_t *p = &o[sh_w * y];
            for (int x = 0; x < crop.w; x++, p++)
                *p = fn(*p, x, y);
        }
    }
    
    inline void each(std::function<void(uint8_t &, int, int)> fn) {
        assert(pixels.c == Type::ui8 || pixels.c == Type::i8);
        auto    sh   = pixels.shape();
        int     sh_d = int(sh[2]);
        int     sh_w = int(sh[1]);
        int     sh_h = int(sh[0]);
        assert( sh_d == 1 );
        recti   crop = pixels.count("crop") ? recti(pixels["crop"]) : recti { 0, 0, sh_w, sh_h };
        uint8_t   *o = (uint8_t *)pixels.data<uint8_t>() + crop.y * sh_w + crop.x;
        for (int   y = 0; y < crop.h; y++) {
            uint8_t *p = &o[sh_w * y];
            for (int x = 0; x < crop.w; x++, p++)
                fn(*p, x, y);
        }
    }
};

void shodgeman(array<vec2> &poly, array<vec2> &clip);

template<> struct is_strable<Image> : std::true_type  {};
