#pragma once
#include <ux/node.hpp>

enums(Rendering, Type, None, None, Shader, Wireframe);

template <typename V>
struct Object:node {
    declare(Object);
    
    struct Members {
        Extern<Path>           model;
        Extern<Shaders>        shaders;
        Extern<UniformData>    ubo;
        Extern<array<Path>>    textures;
        Extern<Rendering>      render;
        Intern<Pipes>          pipes;
    } m;

    void bind() {
        external("uniform",  m.ubo,        UniformData    { null     });
        external("model",    m.model,      Path           { ""       });
        external("shaders",  m.shaders,    Shaders        { "*=main" });
        external("textures", m.textures,   array<Path>    {          });
        external("render",   m.render,     Rendering      { Rendering::Shader });
        internal("pipes",    m.pipes,      Pipes          { null     });
    }
    
    void changed(PropList list) {
        if (list.count("model") == 0)
            return;
        m.pipes = model<Vertex>(m.model, m.ubo, m.textures, m.shaders);
    }
    
    Element render() {
        //return node::render();
        auto &device = Vulkan::device();
        Pipes &pipes = m.pipes;
        if (pipes) for (auto &[name, pipe]: pipes.map())
            device.render.push(pipe);
        return node::render();
    }
};
