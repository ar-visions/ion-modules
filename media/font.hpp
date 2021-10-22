#pragma once

#include <data/data.hpp>
#include <media/obj.hpp>

struct FontData;
struct Font {
    std::string           alias;
    double                sz;
    path_t path;
    FontData             *data;

    Font();
    Font(std::string alias, double sz = 0, path_t = "");
    Font(std::string alias, double sz, path_t path, FontData *data);
};
