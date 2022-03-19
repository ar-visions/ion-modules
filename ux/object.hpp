#pragma once
#include <ux/node.hpp>

/// a type registered enum, with a default value
enums(Rendering, Type, None, None, Shader, Wireframe);

template <typename V>
struct Object:node {
    declare(Object);
    
    /// our external interface
    struct Members {
        Extern<Path>           model;
        Extern<Shaders>        shaders;
        Extern<UniformData>    ubo;
        Extern<array<Path>>    textures;
        Extern<Rendering>      render;
    } m;
    
    Pipes   pipes;
    
    /// declare members their default values
    void bind() {
        external("uniform",  m.ubo,        UniformData    { null     });
        external("model",    m.model,      Path           { ""       });
        external("shaders",  m.shaders,    Shaders        { "*=main" });
        external("textures", m.textures,   array<Path>    {          });
        external("render",   m.render,     Rendering      { Rendering::Shader });
    }
    
    /// change management
    void changed(PropList list) {
        if (!list.count("model"))
            return;
        pipes = model<Vertex>(m.model, m.ubo, m.textures, m.shaders);
    }
    
    /// rendition of pipes
    Element render() {
        if (pipes) for (auto &[name, pipe]: pipes.map())
            device().render.push(pipe);
        return node::render();
    }
};
