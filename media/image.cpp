#include <media/image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Image::Image(nullptr_t n) { }

Image::Image(rgba *data, vec3i shape, size_t stride, bool alloc) :
    data(data), shape(shape), stride(stride), alloc(alloc) { }

Image::Image(std::filesystem::path p) {
    data = (rgba *)stbi_load(p.string().c_str(),
        &shape.x, &shape.y, nullptr, 4);
    assert(data);
    shape.z = 4;
    stride = shape.x * shape.z;
    alloc = true;
}

Image Image::crop(recti r) {
    return { &data[stride * int(r.y) + int(r.x)], { r.w, r.h, 4 }, stride };
}

Image::~Image() {
    if (alloc)
        stbi_image_free((stbi_uc *)data);
}

bool Image::operator!() {
    return !data;
}

Image::operator bool() {
    return data != null;
}


