#pragma once
#include <ux/style.hpp>
#include <ux/node.hpp>
//#include <ux/ux.hpp>

struct Rendering:ex<Rendering> {
    enum Type { None, Shader, Wireframe };
    static Symbols symbols;
    Rendering(Type t = None):ex<Rendering>(t) { }
    Rendering(std::string s):ex<Rendering>(None) {
        kind = resolve(s);
    }
};

template <typename V>
struct Object:node {
    declare(Object);
    
    struct Members {
        Extern<path_t>      model;
        Extern<Shaders>     shaders;
        Extern<UniformData> ubo;
        Extern<vec<Attrib>> attr;
        Extern<Rendering>   render;
        /// --------------------------
        Intern<str>         sval;
        Intern<int>         ivar;
        Intern<Pipes>       pipes;
    } m;

    ///
    void bind() {
        external("uniform", m.ubo,       UniformData   { null     });
        external("model",   m.model,     path_t        { ""       });
        external("shaders", m.shaders,   Shaders       { "*=main" });
        external("attr",    m.attr,      vec<Attrib>   {          });
        external("render",  m.render,    Rendering     { Rendering::Shader });
        /// ------------------------------------------------
        internal("pipes",   m.pipes,     Pipes         { null });
    }
    
    ///
    void changed(PropList list) {
        if (list.count("model") == 0 || !m.attr())
            return;
        path_t &mod = m.model;
        m.pipes     = model<Vertex>(mod, m.ubo, m.attr, m.shaders);
    }
    
    /// dont make anything difficult on yourself. at all kalen.
    Element render() {
        auto &device = Vulkan::device();
        Pipes &pipes = m.pipes;
        if (pipes)
            for (auto &[name, pipe]: pipes.map())
                device.render.push(pipe);
        return null;
    }
};

