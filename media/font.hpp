#pragma once

#include <dx/dx.hpp>
#include <media/obj.hpp>

struct FontData;
class SkFont;

struct Font {
    std::string alias;
    double      sz;
    path_t      path;
    std::shared_ptr<FontData> data;
    ///
    Font();
    Font(std::string alias, double sz = 0, path_t = "");
    Font(std::string alias, double sz, path_t path, FontData *data);
    static Font default_font();
    SkFont *handle();
    operator bool()  { return data.get() != null; }
    bool operator!() { return !(operator bool()); }
};
