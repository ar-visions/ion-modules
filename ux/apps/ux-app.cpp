#include <ux/ux.hpp>
#include <ux/app.hpp>
#include <media/obj.hpp>


struct Sample:node {
    declare(Sample);
    
    struct Props:IProps {
        str model;
    } props;
    
    enum Uniform {
        U_MVP
    };
    
    void define() {
        Define <str> {this, "model", &props.model, "models/dayna2.obj"};
    }
    
    // typed<T>
    
    Element render() {
        return
            Group ({{"border-color", "#ff0"}, {"border-size", 1.0}}, {
                Label ({
                    {"text-label",   "inline model component"},
                    {"text-align-x",  Align::Start},
                    {"region",        Region {L{1}, T{1}, W{1.0}, H{1.0}}}
                }),
                Object ({
                    {"region",        Region {L{1}, T{3}, R{1.0}, B{1.0}}   },
                    {"model",         path_t(props.model)                   },
                    {"shaders",       Args {{"*", "main"}}                  },
                    {"uniform",       uniform<MVP>(U_MVP, [&](MVP &mvp) {
                        /// this method is called before the uniform buffer is transferred
                        /// perform very minimal initialization in uniform
                        /// lambda is passed into uniform prop, it must receive a UniformFn as well as the uniform id
                        /// currently it is
                        ///
                    })}
                })
            });
    }
};

Args defaults = {
    {"window-width",  int(1024) },
    {"window-height", int(1024) },
    {"model",         str("models/dayna.obj") }
};

int main(int c, const char *v[]) {
    return App::UX(c, v, defaults)([&] (Args &args) {
        return Sample(args);
    });
}
