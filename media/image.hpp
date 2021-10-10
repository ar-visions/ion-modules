#pragma once
#include <data/data.hpp>
#include <data/vec.hpp>
#include <media/color.hpp>

struct Image {
    Image(nullptr_t n = null);
    Image(std::filesystem::path);
    Image(rgba *, vec3i, size_t, bool = false);
   ~Image();
    Image crop(recti);
    rgba *data = null;
    vec3i shape;
    size_t stride = 0;
    bool alloc = false;
    bool operator!();
    operator bool();
    inline void each(std::function<rgba(rgba &, int, int)> fn, rectd r) {
        rgba *d = &data[stride * int(r.y) + int(r.x)];
        for (int y = 0; y < r.h; y++) {
            rgba *p = &d[stride * y];
            for (int x = 0; x < r.w; x++, p++)
                *p = fn(*p, x, y);
        }
    }
};
