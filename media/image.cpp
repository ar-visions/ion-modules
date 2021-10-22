#include <media/image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image::Image(nullptr_t n) { }
Image::Image(var &pixels) : pixels(pixels) { }

Image::Image(path_t p, Image::Format f) {
    bool  rgba = (f == Format::Rgba);
    vec3i   sh = { 0, 0, rgba ? 4 : 1 };
    uint8_t *m = (uint8_t *)stbi_load(p.string().c_str(), &sh.x, &sh.y, null, sh.z);
    auto    sp = std::shared_ptr<uint8_t>(m);
    pixels     = var(sp, std::vector<int> { sh.y, sh.x, 1 });
    pixels.c   = rgba ? var::ui32 : var::ui8;
    assert(pixels.size());
}

// it holds onto a reference crop inside of the pixel data
Image::operator var() {
    return pixels;
}

size_t Image::stride() {
    size_t c = pixels.c == var::ui32 ? 4 : 1;
    return pixels.sh.size() >= 2 ? size_t(pixels.sh[1]) * c : 0;
}

void Image::save(path_t p) {
    auto        e = p.extension();
    int         w = pixels.sh[1];
    int         h = pixels.sh[0];
    recti       r = pixels.count("crop") ?
            recti { pixels["crop"] } :
            recti { 0, 0, w, h };
    size_t     st = stride();
    int         c = pixels.c == var::ui32 ? 4 : 1;
    char       *d = (char *)calloc(r.h, r.w * c);
    uint8_t   *px = &pixel<uint8_t>(0, 0);
    size_t  rsize = r.w * c;
    char       *s = d;
    
    for (int i = 0; i < r.h; i++, s += rsize, px += st)
        memcpy(s, px, rsize);
    
    if      (e == ".png")
        stbi_write_png(p.c_str(), r.w, r.h, c, d, rsize);
    else if (e == ".jpg" ||
             e == ".jpeg") {
        stbi_write_jpg(p.c_str(), r.w, r.h, c, d, 100);
    } else
        assert(false);
    
    free(d);
}

Image Image::crop(recti r) {
    auto     sh = pixels.shape(); // data shape!.. we have it.
    const int w = sh[1],
              h = sh[0];
    recti    cr = pixels.count("crop") ?
                    recti { pixels["crop"] } :
                    recti { 0, 0, w, h },
             rr = { r.x + cr.x,
                    r.y + cr.y,
                    r.w, r.h };
    bool copy   = (rr.x < 0 || rr.x + rr.w > w) ||
                  (rr.y < 0 || rr.y + rr.h > h);
    // check if its out of bounds
    // if so, a copy must be made
    assert(!copy);
    var result = pixels.tag({{"crop", rr}});
    recti rrect = result["crop"];
    int test = 0;
    test++;
    return result;
}

Image::~Image() {
    //if (pixels && pixels.d.use_count() == 1)
    //    stbi_image_free((stbi_uc *)pixels.d.get());
}

Image::operator bool()  { return pixels; }
bool Image::operator!() { return !(operator bool()); }

