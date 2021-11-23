#pragma once
#include <dx/dx.hpp>

typedef size_t IPos;
typedef size_t INormal;
typedef size_t IUV;

template <typename T>
struct Obj {
    struct Part {
        str name;
        vec<uint32_t> ibo;
        size_t faces;
        Part() { }
        bool operator!() { return !name; }
        operator bool()  { return  name; }
    };
    
    vec<T> vbo;
    map<str, Part> parts;
    
    Obj(nullptr_t n = nullptr) { }
    Obj(path_t p,
           std::function<T(Part&, vec3&, vec2&, vec3&)> fn)
    {
        str g;
        str contents  = p;
        assert(contents.length() > 0);
        
        auto lines    = contents.split("\n"); // todo: clean this one up.
        auto line_c   = lines.size();
        auto wlines   = vec<Strings>();
        auto v        = vec<vec3>(line_c);
        auto vn       = vec<vec3>(line_c);
        auto vt       = vec<vec2>(line_c);
        auto indices  = map<str, uint32_t>();
        size_t verts  = 0;
        
        for (auto &l:lines)
            wlines += l.split(" ");
        
        // assert triangles as you count them
        for (auto &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                parts[g].name  = g;
                parts[g].faces = 0;
            } else if (w[0] == "f") {
                assert(g.length() && w.size() == 4);
                parts[g].faces++;
            }
        }
        
        // add vertex data to temporary stack lists
        for (auto &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                if (!parts[g].ibo)
                     parts[g].ibo = vec<uint32_t>(parts[g].faces * 3);
            }
            else if (w[0] == "v")  v  += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "vt") vt += vec2 { w[1].real(), w[2].real() };
            else if (w[0] == "vn") vn += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "f") {
                assert(g.length());
                for (size_t i = 1; i < 4; i++) {
                    auto key = w[i];
                    if (indices.count(key) == 0) {
                        indices[key] = uint32_t(verts++);
                        auto  sp =  w[i].split("/");
                        int  iv  = sp[0].integer();
                        int  ivt = sp[1].integer();
                        int  ivn = sp[2].integer();
                        vbo += fn(parts[g], iv  ?  v[ iv-1]:vec3::null(),
                                            ivt ? vt[ivt-1]:vec2::null(),
                                            ivn ? vn[ivn-1]:vec3::null());
                    }
                    parts[g].ibo += indices[key];
                }
            }
        }
    }
};
