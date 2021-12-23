#include <ux/ux.hpp>
#include <ux/app.hpp>
#include <media/obj.hpp>
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective

///component works to perform its own occlusion techniques
struct Car:node {
    Car(Binds binds = {}, vec<Element> elements = {}):
        node((const char *)"", (const char *)"Car", binds, elements) { }
        inline static Car *factory() { return new Car(); };
        inline operator  Element() {
            return {
                FnFactory(Car::factory),
                node::binds,
                elements
            };
        }
    
    enum Uniform { U_MVP };

    struct Members {
        Extern<str>         model;
        Extern<real>        fov;
        Intern<UniformData> uniform;
    } m;
    
    void binds() {
        external<str>  ("model",   m.model,  "models/dayna.obj");
        external<real> ("fov",     m.fov,     60.0);
        
        internal<UniformData>("uniform", m.uniform,
            uniform<MVP>(int(U_MVP), [&](MVP &mvp) {
                mvp.model = glm::mat4(1.0);
                mvp.view  = glm::mat4(1.0);
                mvp.proj  = glm::perspective(
                    float(radians(real(m.fov))),
                    float(path.aspect()),
                    1.0f, 100.0f
                );
            })
        );
    }

    /// render car, smooth shaded
    // we need array support in texture
    Element render() {
        return Object<Vertex> {{
            {"model"}, {"uniform"}
        }};
    }
};

// Member could be brought to higher significance
// So binds over args, and depth lookups used with context

Args defaults = {
    {"window-width",  int(1024) },
    {"window-height", int(1024) },
    {"model",         str("models/dayna.obj") }
};

int main(int c, const char *v[]) {
    return App::UX(c, v, defaults)([&] (Args &args) {
        /// we find ourselves with no Args to Binds interfacing at root.
        /// this is best to add to UX general, folding in this lambda
        return Car();
    });
}
