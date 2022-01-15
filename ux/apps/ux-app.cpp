#include <ux/app.hpp>
#include <ux/object.hpp>
#include <ux/button.hpp>

struct Car:node {
    declare(Car);
    enum Uniform { U_MVP };
    ///
    struct Members {
        Extern<path_t>      model;
        Extern<real>        fov;
        Extern<Rendering>   render;
        Lambda<VAttr, Car>  attr;
        Intern<UniformData> uniform;
    } m;
    ///
    static VAttr attr(Car &c) {
        return {};
    }

    /// [ ] spinning car
    /// [ ] live reload on shaders
    /// [ ] jet plume shader

    void bind() {
        external<path_t>     ("model",   m.model,  "egon");
        external<real>       ("fovs",    m.fov,     60.0);
        external<Rendering>  ("render",  m.render, { Rendering::Shader });
        lambda  <VAttr, Car> ("attr",    m.attr,   [](Car &c) -> VAttr { return VAttr(); });
        
        internal<UniformData> ("uniform", m.uniform,
            uniform<MVP>(int(U_MVP), [&](MVP &mvp) {
                mvp.model = glm::mat4(1.0);
                mvp.view  = glm::mat4(1.0);
                mvp.proj  = glm::perspective(
                    float(radians(real(m.fov))), float(paths.fill.aspect()), 1.0f, 100.0f);
            }));
        ///
    }
    
    Element render() {
        return Object<Vertex> {{
            {"render"}, {"attr"}, {"model"}, {"uniform"}
        }};
    }
};


struct View:node {
    declare(View);
    ///
    struct Members {
        Extern<path_t> model;
        Extern<Region> button_region;
        Intern<str>    button_label;
        Intern<rgba>   button_fill;
    } m;
    
    /// support live reloads on css and shaders.
    void bind() {
        external("model",  m.model,  path_t {"dayna"}); /// just 'dayna' should work
        /// -------------------------------------------
        override(node::m.ev.hover,   Fn([&](Event ev) { // no type set on hover Member. that would of been done in standard()
            console.log("Good morning, Dave...");
        }));
    }
    
    Element render() {
        return Group {
            {{"id", "something"}}, {
                Button {{ {"id", "button"} }},
                Car    {{ {"id", "car"}    }}
            }
        };
    }
};

Args defaults = {
    {"window-width",  int(1024)},
    {"window-height", int(1024)},
    {"model",         path_t("models/ray.obj")}
};

int main(int c, const char *v[]) {
    return UX<View>(c, v, defaults);
}
