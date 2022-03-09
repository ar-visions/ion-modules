#include <ux/app.hpp>
#include <ux/object.hpp>
#include <ux/button.hpp>

/// [ ] left-oriented vertical toolbar
/// [ ] top button
/// [x] button right watermark


///
typedef map<str, Element> ViewMap;

///
struct View:node {
    declare(View);
    ///
    struct Members {
        Extern<Path>   model;   /// do not make the generics complex to facilitate std library, use pass-through abstract types here
        Extern<Region> button_region;
        Intern<str>    button_label;
        Intern<rgba>   button_fill;
    } m;
    
    void bind() {
        external("model",  m.model,  Path("dayna"));
        /// -------------------------------------------
        override(node::m.ev.hover,   Fn([&](Event ev) {
            console.log("Good morning, Dave...");
        }));
    }
    
    Element render() { return Group("something", {}, { Button("button") }); }
};

/// render Mars.
struct Mars:node {
    declare(Mars);
    ///
    struct Members {
        Extern<real>          dx, dy;
        Extern<real>          fov;
        Extern<Rendering>     render;
        Extern<Path>          model;
        Extern<UniformData>   uniform;
        Extern<array<Path>>   textures;
    } m;
    
    /// we need to make this open-ended enough to work.
    void bind() {
        external<real>          ("fovs",    m.fov,     60.0);
        external<Rendering>     ("render",  m.render, { Rendering::Shader });
        external<Path>          ("model",   m.model,  "models/uv-sphere.obj"); /// vattr not needed, vertex defines it.  the only thing we need to map is the texture parameters
        external<UniformData>   ("uniform", m.uniform, uniform<MVP>([&](MVP &mvp) {
            ///
            r32    fov = radians(real(m.fov));
            r32 aspect = paths.fill.aspect();
            mvp.model   = m44f::identity();
            mvp.view    = m44f::identity(); //m44f::translation(vec3f { 0.0f, 0.0f, -1.0f });
            //mvp.proj    = m44f::perspective(fov, aspect, vec2f { 1.0f, 100.0f });
            ///
            r32 sc  = 4.0f;
            r32 sx  =   1.0f * 0.5f * sc;
            r32 sy  = aspect * 0.5f * sc;
            mvp.proj = glm::ortho(-sx, sx, -sy, sy, 2.5f, -2.5f);
            
            int test = 0;
            test++;
        }));
        ///
        external<array<Path>>   ("textures", m.textures, {"images/8k_mars.jpg"}); /// these are lazy loaded
    }
    ///
    Element render() {
        return Object<Vertex>("object", {
            {"render", m.render}, {"textures", m.textures},
            {"model",  m.model},  {"uniform",  m.uniform}
        });
    }
};

/// move
struct Shell:node {
    declare(Shell);
    
    ///
    struct Members {
        Extern<ViewMap> views;
        Intern<str>     text_label;
    } m;
    
    ///
    void bind() {
        internal<str>("text-label", m.text_label, "Some button");
    }

    ///
    Element render() {
        return
            Group("base", {}, {
                 Mars("mars"),
                Group("dock", {}, {
                    Element::filter<str>({"hi", "hi2"}, [&](str &v) {
                        return Button(v, {{"text-label", m.text_label}});
                    })
                })
            });
    }
};

///
Map defaults = {
    { "window-width",     int(1600)             },
    { "window-height",    int(1024)             },
    { "model",         path_t("models/ray.obj") }
};

///
int main(int c, const char *v[]) {
    return UX<Shell>(c, v, defaults);
}
