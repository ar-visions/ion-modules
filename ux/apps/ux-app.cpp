#include <ux/app.hpp>
#include <ux/object.hpp>

struct Mars:node {
    declare(Mars);
    ///
    struct Members {
        Extern<real>          fov;
        Extern<Rendering>     render;
        Extern<Path>          model;
        Extern<UniformData>   uniform;
        Extern<array<Path>>   textures;
    } m;
    ///
    struct Uniform {
        MVP         mvp;
        Material    material;
        Light       lights[2];
    };
    ///
    real degs = 0;
    ///
    void bind() {
        external<real>          ("fov",     m.fov,     60.0);
        external<Rendering>     ("render",  m.render, { Rendering::Shader });
        external<Path>          ("model",   m.model,  "models/mars-8k.obj"); // where are we?
        external<UniformData>   ("uniform", m.uniform, uniform<Uniform>([&](Uniform &u) {
            r32    aspect = 1.0 / paths.fill.aspect();
            r32        sx = 1.0f   * 0.5f * 2.0f;
            r32        sy = aspect * 0.5f * 2.0f;
            u.mvp.model   = glm::rotate(glm::mat4(1.0f), float(degs += 0.001), glm::vec3(0.0f, 1.0f, 0.0f)),
            u.mvp.view    = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 6.0f));
            u.mvp.proj    = glm::ortho(-sx, sx, -sy, sy, 200.0f, -200.0f);
            u.material    = {
                .ambient  = { 0.05, 0.05, 0.05, 1.00 },
                .diffuse  = { 1.00, 1.00, 1.00, 1.00 },
                .specular = { 1.00, 1.00, 1.00, 1.00 },
                .attr     = { 2.00, 0.00, 0.00, 0.00 }};
            u.lights[0]   = {
                .pos_rads = { -1000.0f, 0.0f, 0.0f, 10000.0f },
                .color    = {     1.0f, 1.0f, 1.0f, 0.0f }};
            u.lights[1]   = {
                .pos_rads = { 0.0f, 0.0f, -1000.0f, 10000.0f },
                .color    = { 1.0f, 1.0f,  1.0f, 0.0f }};
        }));
    }
    ///
    Element render() {
        return Object<Vertex> {"object", {
            {"render", m.render}, {"model",  m.model}, {"uniform", m.uniform}
        }};
    }
};

struct Pane:node {
    declare(Pane);
};

struct Avatar:node {
    declare(Avatar);
};

/// sorry but i havent been working enough.  here is Orbiter
struct Orbiter:node {
    declare(Orbiter);
    
    void bind() {
        //external <real> ("fov", m.fov, 60.0);
    }

    Element render() {
        return Pane {
            "orbiter", {}, {
                Mars   { "mars" },
                Avatar { "avatar" }
            }
        };
    }
};

///
Map defaults = {
    { "window-width",     int(1600) },
    { "window-height",    int(1024) }
};

///
int main(int c, const char *v[]) {
    return UX<Orbiter>(c, v, defaults);
}
