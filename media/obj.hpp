#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>

typedef size_t IPos;
typedef size_t INormal;
typedef size_t IUV;

template <typename T>
struct Obj {
    struct Group {
        str name;
        array<uint32_t> ibo;
        size_t faces;
        Group() { }
        bool operator!() { return !name; }
        operator bool()  { return  name; }
    };
    
    array<T> vbo;
    map<str, Group> groups;
    
    Obj(std::nullptr_t n = nullptr) { }
    Obj(path_t p,
           std::function<T(Group&, vec3&, vec2&, vec3&)> fn)
    {
        str g;
        str contents  = str::read_file(std::filesystem::exists(p) ? p
            : path_t(var::format("models/{0}.obj", {p})));
        assert(contents.size() > 0);
        
        auto lines    = contents.split("\n");
        auto line_c   = lines.size();
        auto wlines   = array<Strings>();
        auto v        = array<vec3>(line_c);
        auto vn       = array<vec3>(line_c);
        auto vt       = array<vec2>(line_c);
        auto indices  = map<str, uint32_t>();
        size_t verts  = 0;
        ///
        for (auto &l:lines)
            wlines += l.split(" ");
        ///
        /// assert triangles as we count them
        for (auto &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                groups[g].name  = g;
                groups[g].faces = 0;
            } else if (w[0] == "f") {
                assert(g.size() && w.size() == 4); /// f pos/uv/norm pos/uv/norm pos/uv/norm
                groups[g].faces++;
            }
        }
        /// add vertex data
        for (auto &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                if (!groups[g].ibo)
                    groups[g].ibo = array<uint32_t>(groups[g].faces * 3);
            }
            else if (w[0] == "v")  v  += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "vt") vt += vec2 { w[1].real(), w[2].real() };
            else if (w[0] == "vn") vn += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "f") {
                assert(g.size());
                for (size_t i = 1; i < 4; i++) {
                    auto key = w[i];
                    if (indices.count(key) == 0) {
                        indices[key] = uint32_t(verts++);
                        auto  sp =  w[i].split("/");
                        int  iv  = sp[0].integer();
                        int  ivt = sp[1].integer();
                        int  ivn = sp[2].integer();
                        vbo += fn(groups[g], iv ?  v[ iv-1]:vec3::null(),
                                            ivt ? vt[ivt-1]:vec2::null(),
                                            ivn ? vn[ivn-1]:vec3::null());
                    }
                    groups[g].ibo += indices[key];
                }
            }
        }
    }
};
