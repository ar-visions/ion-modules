#pragma once

#include <data/data.hpp>
#include <media/obj.hpp>

struct FontData;
struct Font {
    std::string           alias;
    double                sz;
    std::filesystem::path path;
    FontData             *data;

    Font();
    Font(std::string alias, double sz = 0, std::filesystem::path = "");
    Font(std::string alias, double sz, std::filesystem::path path, FontData *data);
};
