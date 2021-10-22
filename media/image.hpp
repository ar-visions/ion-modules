#pragma once
#include <data/data.hpp>
#include <data/vec.hpp>
#include <media/color.hpp>

struct Image {
    enum Format {
        Gray,
        Rgba
    };
    
    Image(nullptr_t n = null);
    Image(path_t, Image::Format);
    Image(var &);
   ~Image();
    Image      crop(recti);
    operator   var();
    void       save(path_t);
    size_t   stride();
    bool  operator!();
    operator   bool();
    operator  recti();
    operator  vec2i();
    
    template <typename T>
    T &pixel(int x, int y) {
        auto    sh = pixels.sh;
        vec2i orig = pixels.count("crop") ? vec2i(pixels["crop"]) : vec2i { 0, 0 };
        return *(pixels.data<T>() + orig.y * sh[1] + orig.x);
    }
    
    var  pixels;
    
    inline void each(std::function<rgba(rgba &, int, int)> fn) {
        assert(pixels.c == var::ui32 || pixels.c == var::i32);
        auto    sh = pixels.shape();
        assert(sh[2] == 4);
        recti crop = pixels.count("crop") ? recti(pixels["crop"]) : recti { 0, 0, sh[1], sh[0] };
        rgba    *o = (rgba *)pixels.data<uint32_t>() + crop.y * sh[1] + crop.x;
        for (int y = 0; y < crop.h; y++) {
            rgba *p = &o[sh[1] * y];
            for (int x = 0; x < crop.w; x++, p++)
                *p = fn(*p, x, y);
        }
    }
    
    inline void each(std::function<uint8_t(uint8_t &, int, int)> fn) {
        assert(pixels.c == var::ui8 || pixels.c == var::i8);
        auto    sh = pixels.shape();
        assert(sh[2] == 1);
        recti crop = pixels.count("crop") ? recti(pixels["crop"]) : recti { 0, 0, sh[1], sh[0] };
        uint8_t *o = (uint8_t *)pixels.data<uint8_t>() + crop.y * sh[1] + crop.x;
        for (int y = 0; y < crop.h; y++) {
            uint8_t *p = &o[sh[1] * y];
            for (int x = 0; x < crop.w; x++, p++)
                *p = fn(*p, x, y);
        }
    }
};
