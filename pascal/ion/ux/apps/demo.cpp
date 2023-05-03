#include <core/core.hpp>
#include <math/math.hpp>
#include <net/net.hpp>
#include <async/async.hpp>
#include <ux/ux.hpp>

struct view:node {
    struct props {
        int sample;
        callback handler;
    } &m;
    
    ///
    ctr_args(view, node, props, m);
    
    ///
    doubly<prop> meta() {
        return {
            prop { m, "sample",  m.sample },
            prop { m, "handler", m.handler}
        };
    }
    
    ///
    void mounting() {
        console.log("mounting");
    }

    /// if no render is defined, the content is used for embedding children from content (if its there)
    /// if there is a render the content can be used within it
    Element render() {
        return button {
            { "content", fmt {"hello world: {0}", { m.sample }} },
            { "on-click",
                callback([&](event e) {
                    console.log("on-click...");
                    if (m.handler)
                        m.handler(e);
                })
            }
        };
    }
};

/// add callback functionality into the args; should be there but isnt.
int main(int argc, cstr argv[]) {
    return app([](app &ctx) -> Element {
        return view {
            { "id",     "main"  }, /// id should be a name of component if not there
            { "sample",  int(2) },
            { "on-silly", callback([](event e) {
                console.log("on-silly");
            })}
        };
    });
}



























//module app;

/*
    arg a;
    view v1 = view {ax {{"abc", mx(123)}}};
    return app([](app &ctx) -> Element {
        return view("");
    });
struct mars:node {
    declare(mars);
    ///
    struct Members {
        real          fov;
        Rendering     render;
        str           model;
        UniformData   uniform;
        Shaders       shaders;
        Asset::Types  assets; /// shaders are more than assets, not less.
        array<path>   textures;
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

///
map<mx> defaults = {
    { "window-width",  int(1600) },
    { "window-height", int(1024) }
};

/// compile for console, and gui.

int main(int c, const char *v[]) {
    return app<view>(c, v, defaults);
}
*/


/// app must compile differently depending on build args: build.console, build.service, build.gui [vulkan included]
#if 0
struct view:node {

    struct model {
        str something;
    };

    ctr(view, node, model, m);

    view(memory *m) : mx(m) { }
    view()          : view(mx::alloc<model>()) {
        /// one could know if all members are mapped in struct

    }
};
#endif