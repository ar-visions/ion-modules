#include <ux/ux.hpp>
#include <ux/app.hpp>
#include <media/obj.hpp>

/// simple rendering
struct Car:node {
    declare(Car);
    enum Uniform { U_MVP };

    struct Members {
        Extern<str>         model;
        Extern<real>        fov;
        /// ------------------------
        Intern<UniformData> uniform;
    } m;
    
    /// configure binds
    void binds() {
        external("model",   m.model,   str {"models/dayna.obj"});
        external("fov",     m.fov,     60.0);
        /// ---------------------------------
        internal("uniform", m.uniform,
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

    /// render Component with simple binding syntax
    /// this is much like having the same value for a field in ES6 field:field -> field
    /// if there are different dest name you have two strings {"external-bind","internal-bind"}
    Element render() {
        return Object<Vertex> {{
            {"model"}, {"uniform"}
        }};
    }
};

Args defaults = {
    {"window-width",  int(1024) },
    {"window-height", int(1024) },
    {"model",         str("models/dayna.obj") }
};

int main(int c, const char *v[]) {
    return App::UX<Car>(c, v, defaults);
}
