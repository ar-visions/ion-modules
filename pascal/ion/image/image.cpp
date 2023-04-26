#define _CRT_SECURE_NO_WARNINGS
#include "stb-image-read.h"
#include "stb-image-write.h"

#include <image/image.hpp>

/// load an image into 32bit rgba format
image::image(size sz, rgba::data *px, int scanline) : array() {
    mem->shape  = new size(sz);
    mem->origin = px;
    elements    = px;
    assert(scanline == 0 || scanline == sz[1]);
}

/// save image, just png out but it could look at extensions too
bool image::save(path p) const {
    assert(mem->shape && mem->shape->dims() == 2);
    int w = int(width()), 
        h = int(height());
    return stbi_write_png(p.cs(), w, h, 4, elements, w * 4);
}


/// load an image into 32bit rgba format
image::image(path p) : array() {
    int w = 0, h = 0, c = 0;
    /// if path exists and we can read read an image, set the size
    rgba::data *data = null;
    if (p.exists() && (data = (rgba::data *)stbi_load(p.cs(), &w, &h, &c, 4))) {
        mem->count  = h * w;
        mem->shape  = new size { h, w };
        mem->origin = data; /// you can set whatever you want here; its freed at end of life-cycle for memory
        elements    = data;
    }
}
