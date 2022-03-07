#include <media/image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image::Image(std::nullptr_t n) { }
Image::Image(var &pixels) : pixels(pixels) { /// images/ implied, probably will do the same
    if (pixels == Type::Str) {
        bool no_ext = path_t(pixels).extension() == "";
        path_t path = var::format("images/{0}{1}", {pixels, (no_ext ? str {".png"} : str {""})});
        *this = Image(path, Format::Rgba);
    }
}

Image::Image(path_t p, Image::Format f) {
    bool     g = f != Rgba;
    vec3i   sh = { 0, 0, g ? 1 : 4 };
    uint8_t *m = (uint8_t *)stbi_load(p.string().c_str(), &sh.x, &sh.y, null, sh.z);
    pixels     = var { g ? Type::ui8 : Type::ui32, std::shared_ptr<uint8_t>(m), Shape<Major>(sh.y, sh.x, g ? 1 : 4) };
    assert(pixels.size());
}

Image::Image(vec2i sz, Format f) {
    auto g = f == Gray;
    pixels     = var { g ? Type::ui8 : Type::ui32, Shape<Major>(sz.y, sz.x, g ? 1 : 4) };
    // make sure this is not used in vector form.  all shapes! they be shapes.
    //pixels = {g ? Type::ui8 : Type::ui32,
    //          std::vector<int> { sz.y, sz.x, g ? 1 : 4 }};
}

Image::Image(vec2i sz, rgba clr) {
    pixels = var { Type::ui32, Shape<Major>::rgba(sz.x, sz.y) };
}

Image::Format Image::format() {
    return pixels.c == Type::ui32 ? Rgba : Gray;
}

vec2 Image::size() {
    return vec2 { real(pixels.sh[1]), real(pixels.sh[0]) };
}

Image Image::resample(vec2i sz) {
    vec2 sc = vec2(sz) / vec2(size());
    m44   m = m44().scale(vec2(1.0) / sc);
    return transform(m, sz);
}

Image Image::transform(m44 m, vec2i vp) {
    Format    f = format();
    Image    vi = Image(vp, f);
    vec2     sz = size();
    rectd     r = {0.0, 0.0, sz[0], sz[1]};
    array<vec2> clip = {{r.x,r.y}, {r.x + r.w,r.y},
                      {r.x + r.w,r.y + r.w}, {r.x,r.y + r.h}};
    
    for (    int y = 0; y < vp.y; y++)
        for (int x = 0; x < vp.x; x++)
            vi.pixel<uint8_t>(x, y) = 0;
    
    switch (f) {
        case Image::Gray: {
            auto tx = vi.pixels.data<uint8_t>();
            for     (int y = 0; y < vp.y; y++) {
                for (int x = 0; x < vp.x; x++) {
                    vec4  h = m * vec4(x + 0.5, y + 0.5, 0.0, 1.0);
                    vec2 uv = h.xy() / h.w / sz;
                    *(tx++) = bicubic(uv, sz);
                }
            }
            break;
        }
        case Image::Rgba: {
            auto tx = vi.pixels.data<rgba>();
            for     (int y = 0; y < vp.y; y++) {
                for (int x = 0; x < vp.x; x++) {
                    vec4  h = m * vec4(x + 0.5, y + 0.5, 0.0, 1.0);
                    vec2 uv = h.xy() / h.w / sz;
                    *(tx++) = rgba(bicubic_4(uv, sz));
                }
            }
            break;
        }
    }
    
    return vi;
}

Image::operator var() {
    return pixels;
}

size_t Image::stride() {
    size_t c = pixels.c == Type::ui32 ? 4 : 1;
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
    int         c = pixels.c == Type::ui32 ? 4 : 1;
    char       *d = (char *)calloc(r.h, r.w * c);
    uint8_t   *px = &pixel<uint8_t>(0, 0);
    size_t  rsize = r.w * c;
    char       *s = d;
    
    for (int i = 0; i < r.h; i++, s += rsize, px += st)
        memcpy(s, px, rsize);
    
    if      (e == ".png")
        stbi_write_png(p.c_str(), r.w, r.h, c, d, rsize);
    else if (e == ".jpg" || e == ".jpeg")
        stbi_write_jpg(p.c_str(), r.w, r.h, c, d, 100);
    else
        assert(false);
    
    free(d);
}

Image Image::crop(recti r) {
    auto     sh = pixels.shape();
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
    assert(!copy);
    var result  = pixels.tag({{"crop", rr}});
    return result;
}

Image::~Image() {
    /// we dont know who struck first, us, or them...
    /// we do know it was us, who invalidated the cache.
    /// im not sure why this function is here, and i am standing like keano [massively dumb-founded].
}

Image::operator bool()  { return pixels; }
bool Image::operator!() { return !(operator bool()); }
