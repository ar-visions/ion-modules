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
        Extern<path_t> model;
        Extern<Region> button_region;
        Intern<str>    button_label;
        Intern<rgba>   button_fill;
    } m;
    
    void bind() {
        external("model",  m.model,  path_t {"dayna"});
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
    /// [-] jet plume shader

    void bind() {
        external<path_t>      ("model",   m.model,  "egon");
        external<real>        ("fovs",    m.fov,     60.0);
        external<Rendering>   ("render",  m.render, { Rendering::Shader });
        lambda  <VAttr, Car>  ("attr",    m.attr,   [](Car &c) -> VAttr { return VAttr(
                                                                            /// where the hell were we here, christ.
                                                                            ); });
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

/// you produce builds for console or build for w/3D/AR
/// role is an enum, we decided on this before
/// node works in principle with console, window or cloud role
/// general app menu system, used by Orbiter.
///
/// it has manages what view is selected, its context params given and dialogs
///
/// drawing ops
/// ------------------------------------
/// [ ] left-oriented vertical toolbar
/// [ ] top button
///     -> icon is menu8
/// [ ] button right watermark
///     -> ion module symbol, white, text under optional, version
///

/// As a design-time rule, no ability to hold Element on member, just to keep the relationships trivial
struct Shell:node {
    declare(Shell);
    
    ///
    struct Members {
        Extern<ViewMap> views; // fix this around ambiguity related to size_t
        Intern<str>     button_text;
    } m;
    
    ///
    void bind() {
        internal<str>("button-text", m.button_text, "Some button");
    }
    
    //Element::filter<str>({"hi","hi2"}, [&](str &v) {
    //    return Button(v, {{"text-label", m.button_text}});
    //})
    
    ///
    Element render() {
        return Group("dock", {}, { /// better to have id, attribs, and children. i think
            Button("butt", {{"text-label", str("some text!")}})
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
