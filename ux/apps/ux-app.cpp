#include <ux/app.hpp>
#include <ux/object.hpp>
#include <ux/button.hpp>

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

///
struct Car:node {
    declare(Car);
    enum Uniform { U_MVP };
    ///
    struct Members {
        Extern<Path>        model;
        Extern<real>        fov;
        Extern<Rendering>   render;
        Lambda<VAttr, Car>  attr;
        Intern<UniformData> uniform;
    } m;
    ///
    static VAttr attr(Car &c) { return {}; }

    void bind() {
        external<Path>        ("model",   m.model,  "egon");
        external<real>        ("fovs",    m.fov,     60.0);
        external<Rendering>   ("render",  m.render, { Rendering::Shader });
        lambda  <VAttr, Car>  ("attr",    m.attr,   [](Car &c) -> VAttr { return VAttr(); });
        internal<UniformData> ("uniform", m.uniform,
            uniform<MVP>(int(U_MVP), [&](MVP &mvp) {
                mvp.model = glm::mat4(1.0);
                mvp.view  = glm::mat4(1.0);
                mvp.proj  = glm::perspective(
                    float(radians(real(m.fov))), float(paths.fill.aspect()), 1.0f, 100.0f);
            }));
        ///
    }
    
    /// we can shortcut the binds with a lookup on the caller of the same name, assert that it exists
    Element render() {
        return Object<Vertex>("object", { /// this can still be the short-hand for bind(), it just assumes member with the same name is all.
            {"render", m.render}, {"attr",    m.attr},
            {"model",  m.model},  {"uniform", m.uniform}
        });
    }
};

/// ------------------------------------
/// [ ] left-oriented vertical toolbar
/// [ ] top button
///     -> icon is menu8
/// [ ] button right watermark

/// As a design-time rule, no ability to hold Element on member, just to keep the relationships trivial
struct Shell:node {
    declare(Shell);
    
    ///
    struct Members {
        Extern<ViewMap> views; // fix this around ambiguity related to size_t
        Intern<str>     text_label;
    } m;
    
    ///
    void bind() {
        internal<str>("text-label", m.text_label, "Some button");
    }

    ///
    Element render() {
        return Group("dock", {}, {
            Element::filter<str>({"hi"}, [&](str &v) {
                return Button(v, {{"text-label", m.text_label}});
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
