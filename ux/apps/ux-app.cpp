#include <ux/app.hpp>
#include <ux/object.hpp>
#include <ux/button.hpp>

// macOS doesnt support notifications [/kicks dirt]; get working on Linux and roll into Watch (file, dir)
//#include <sys/inotify.h>

/// lets get this Car node rendering.

/*
/// It was the best term; its an indication of standardization of view and action
struct Shell:node {
    /// dock view set. mimmicks OS but that is ok for our targets
    /// difference maker is the menu reveals more info about the dock items.
    ///
    /// does it need to be combining two things in that way
    /// menu items from dock?
    /// it may not be enough really
    /// i dont want to copy something it has to be based on an idea thats essentially unique.
    /// one thing is for sure we wont have a shitload of views in our prototypical app
    /// whats important is just a central menu, not more than one, ever.
    /// no dialogs.. none of that unless its in-app purchase
    ///s
};*/

struct Car:node {
    declare(Car);
    enum Uniform { U_MVP };

    struct Members {
        Extern<path_t>      model;
        Extern<real>        fov;
        /// ------------------------
        Intern<UniformData> uniform;
    } m;
    
    void bind() {
        external("model",   m.model,   path_t {"models/egon.obj"});
        external("fov",     m.fov,     60.0);
        /// ---------------------------------
        internal("uniform", m.uniform,
            uniform<MVP>(int(U_MVP), [&](MVP &mvp) {
                mvp.model = glm::mat4(1.0);
                mvp.view  = glm::mat4(1.0);
                mvp.proj  = glm::perspective(
                    float(radians(real(m.fov))),
                    float(paths.fill.aspect()),
                    1.0f, 100.0f
                );
            })
        );
    }
    
    Element render() {
        return Object<Vertex> {{
            {"model"}, {"uniform"}
        }};
    }
};


struct View:node {
    declare(View);

    struct Members {
        Extern<path_t> model;
        Extern<Region> button_region;
        Intern<str>    button_label;
        Intern<rgba>   button_fill;
    } m;
    
    /// support live reloads on css and shaders.
    
    void bind() {
        external("model",  m.model,  path_t {"models/dayna.obj"});
        /// -------------------------------------------
        override(node::m.ev.hover,   Fn([&](Event ev) { // no type set on hover Member. that would of been done in standard()
            console.log("Good morning, Dave...");
        }));
    }
    
    Element render() {
        return Button {{
            {"id", "button"}
        }};
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
