#pragma once
#include <ux/style.hpp>
#include <ux/node.hpp>
//#include <ux/ux.hpp>

/// conv to enums() emitter. technical term
struct Rendering:ex<Rendering> {
    static Symbols symbols;
    enum Type { None, Shader, Wireframe };
    ///
    Rendering(Type t = None):ex<Rendering>(t) { }
    Rendering(string s):ex<Rendering>(None)   { kind = resolve(s); }
};

template <typename V>
struct Object:node {
    declare(Object);
    
    ///
    struct Members {
        Extern<Path>           model;
        Extern<Shaders>        shaders;
        Extern<UniformData>    ubo;
        Extern<array<Path>>    textures;
        Extern<Rendering>      render;
        Intern<Pipes>          pipes;
    } m;

    ///
    void bind() {
        external("uniform",  m.ubo,        UniformData    { null     });
        external("model",    m.model,      Path           { ""       });
        external("shaders",  m.shaders,    Shaders        { "*=main" });
        external("textures", m.textures,   array<Path>    {          });
        external("render",   m.render,     Rendering      { Rendering::Shader });
        internal("pipes",    m.pipes,      Pipes          { null     });
    }
    
    /// naming things is all i do.. i dont want to name too much more basic stuff, though.. i think the falcon is starting to lift on her own.
    void changed(PropList list) {
        if (list.count("model") == 0)
            return;
        Path   &mod = m.model;
        m.pipes     = model<Vertex>(mod, m.ubo, m.textures, m.shaders);
    }
    
    /// 
    Element render() {
        auto &device = Vulkan::device();
        Pipes &pipes = m.pipes;
        if (pipes)
            for (auto &[name, pipe]: pipes.map())
                device.render.push(pipe);
        return null;
    }
};

