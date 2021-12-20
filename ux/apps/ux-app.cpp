#include <ux/ux.hpp>
#include <ux/app.hpp>
#include <media/obj.hpp>

///component works to perform its own occlusion techniques
struct Car:node {
    declare(Car);
    
    struct Props:IProps {
        str  model;
        real fov;
    } props;
    
    enum Uniform {
        U_MVP
    };
    
    
    void define() {
        // i suppose we can get rid of this now.
        //Define <str>  { this, "model", &props.model, "models/dayna.obj" };
        //Define <real> { this, "fov",   &props.fov,    60.0 };
    }

    /// render car, smooth shaded
    // we need array support in texture
    Element render() {
        // higher and higher level we go. context for the viewer matrix
        // context.  give us context!
        var f = context("viewer");
        

        // component { name, {{interface,"my_internal"}, {}}
        // the decision to host your own context is what brought this about.
        // its the most simplistic node framework of them all, when it comes to ux expression and overriding ability
        // context, and props.  whaaaaat iiiiis the diifereeeeence?
        // ------------------------------------------------------------
        
        // i want direct bind, and also a filtered bind
        // [ ] binder api, that is very nice.  you create values and you bind them to props.  wow.
        //      -
        //      - its actually better than react. i would imagine people have already expressed similar already too. its too simple.
        //      - what is the best way to express the idea?
        //      - members is the best term i think.
        //      - they can be public and private, public means its a prop.
        //      - but
        //      - implement, now.  vulkan is de good right now.
        // ------------------------------------------------------------
        // styyyyyyylle..  whaaat issss the diifereeeence?
        
        // one function would be props for components
        // one function would be the components
        
        return
            Object<Vertex> ({
                {"region",   Region {L{0}, T{0}, R{0}, B{0}} }, // merely a clipping area
                {"model",    path_t(props.model)             },
                {"shaders",  Args {{"*", "main"}}            },
                {"uniform",  this->uniform<MVP>(int(U_MVP), [&](MVP &mvp) {
                    mvp.model = glm::mat4(1.0); // position and orientation on the vertex model data coming in
                    mvp.view  = glm::mat4(1.0); // the viewer location and orientation, we should look this up via context.
                    mvp.proj  = glm::perspective(radians(props.fov), path.aspect(), 1.0, 100.0); // drugs used  by viewer
                    
                })}
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
        return Car(args);
    });
}
