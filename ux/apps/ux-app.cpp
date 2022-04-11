//#include <ux/app.hpp>
//#include <ux/object.hpp>

module ux.app;

/// we need the canvas working too.. sigh.
struct Mars:node {
    declare(Mars);
    ///
    struct Members {
        Extern<real>          fov;
        Extern<Rendering>     render;
        Extern<str>           model;
        Extern<UniformData>   uniform;
        Extern<Shaders>       shaders;
        Extern<Asset::Types>  assets; /// shaders are more than assets, not less.
        Extern<array<Path>>   textures;
    } m;
    ///
    struct Uniform {
        MVP         mvp;
        Material    material;
        Light       lights[2];
    };
    ///
    real degs = radians(240.0);
    ///
    void bind() {
        external<real>          ("fov",      m.fov,      60.0);
        external<Rendering>     ("render",   m.render,   { Rendering::Shader });
        external<Asset::Types>  ("assets",   m.assets,   { Asset::Color, Asset::Normal, Asset::Displace, Asset::Shine });
        external<str>           ("model",    m.model,    "mars");
        external<Shaders>       ("shaders",  m.shaders,  "*=mars"); // todo: 2 factor blend in generic shader, with colorize map; its what we need for a painting mechanism of sort in AR realm, so putting it in generic works.
        external<UniformData>   ("uniform",  m.uniform,  uniform<Uniform>([&](Uniform &u) {
            r32    aspect = 1.0 / paths.fill.aspect();
            r32        sx = 1.0f   * 0.5f * 2.0f;
            r32        sy = aspect * 0.5f * 2.0f;
            degs         += 0.001;
            u.mvp.model   = m44f::identity().rotate_y(r32(degs));
            u.mvp.view    = context<m44f>("view");
            u.mvp.proj    = m44f::ortho(-sx, sx, -sy, sy, {200.0f, -200.0f});
            
            ///
            /// all of this goes in css. NOW! lol.
            ///
            u.material    = {
                .ambient  = {  0.05,  0.05,  0.05,  1.00 },
                .diffuse  = {  1.00,  1.00,  1.00,  1.00 },
                .specular = {  1.00,  1.00,  1.00,  1.00 },
                .attr     = {  2.00,  0.00,  0.00,  0.00 }};
            u.lights[0]   = {
                .pos_rads = { -10.0f, 0.0f, 0.0f, 10.0f },
                .color    = {   1.0f, 1.0f, 1.0f,  0.0f }};
            u.lights[1]   = {
                .pos_rads = { 0.0f, 0.0f, -1000.0f, 10000.0f },
                .color    = { 1.0f, 1.0f,     1.0f,     0.0f }};
        }));
    }
    ///
    Element render() {
        return Object<Vertex> {"object", {
            {"render", m.render}, {"model",  m.model}, {"uniform", m.uniform} // 19-20s with glm
        }};
    }
};

struct Pane:node {
    declare(Pane);
};


struct Avatar:node {
    declare(Avatar);
    ///
    struct Members {
        Extern<real>          fov;
        Extern<Rendering>     render;
        Extern<str>           model;
        Extern<UniformData>   uniform;
        Extern<Shaders>       shaders;
        Extern<Asset::Types>  assets; /// shaders are more than assets, not less.
        Extern<array<Path>>   textures;
    } m;
    ///
    struct Uniform {
        MVP         mvp;
        Material    material;
        Light       lights[2];
    };
    ///
    real degs = radians(240.0);
    ///
    void bind() {
        external<real>          ("fov",      m.fov,      60.0);
        external<Rendering>     ("render",   m.render,   { Rendering::Shader });
        external<Asset::Types>  ("assets",   m.assets,   { Asset::Color });
        external<str>           ("model",    m.model,    "ion");
        external<Shaders>       ("shaders",  m.shaders,  "*=mars2"); // todo: 2 factor blend in generic shader, with colorize map; its what we need for a painting mechanism of sort in AR realm, so putting it in generic works.
        external<UniformData>   ("uniform",  m.uniform,  uniform<Uniform>([&](Uniform &u) {
            r32    aspect = 1.0 / paths.fill.aspect();
            r32        sx = 1.0f   * 0.5f * 2.0f;
            r32        sy = aspect * 0.5f * 2.0f;
            degs         -= 0.0002;
            u.mvp.model   = m44f::identity().scale(vec4f(2.0f, 2.0f, 2.0f, 1.0f));
            u.mvp.view    = context<m44f>("view");
            u.mvp.proj    = m44f::ortho(-sx, sx, -sy, sy, {200.0f, -200.0f});
            
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

/// sorry but i havent been working enough.  here is Orbiter
struct Orbiter:node {
    declare(Orbiter);
    
    struct Members {
        Lambda<m44f, Orbiter> view;
    } m;
    
    void bind() {
        lambda <m44f, Orbiter> ("view", m.view, [](Orbiter &n) {
            return m44f::identity()
                    .translate(0.0f, 0.0f, 4.0f)
                    .rotate_x (degrees(0.6f))
                    .translate(0.0f, -1.0f, 0.0f);
        });
    }

    Element render() {
        return Pane {
            "orbiter", {}, {
                Mars   { "mars"   },
              //Avatar { "avatar" },
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
