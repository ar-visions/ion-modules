#pragma once
#include <ux/node.hpp>

/// a type registered enum, with a default value
enums(Rendering, Type, None, None, Shader, Wireframe);

template <typename V>
struct Object:node {
    declare(Object);
    
    /// our external interface
    struct Members {
        Extern<str>             model;
        Extern<str>             skin;
        Extern<Asset::Types>    assets;
        Extern<Shaders>         shaders;
        Extern<UniformData>     ubo;
        Extern<Vertex::Attribs> attr;
        Extern<Rendering>       render;
    } m;
    
    Pipes   pipes;
    
    /// declare members their default values
    void bind() {
        external("uniform",   m.ubo,        UniformData     { null });
        external("model",     m.model,      str             { ""   });
        external("skin",      m.skin,       str             { ""   });
        external("shaders",   m.shaders,    Shaders         { "*=main" });
        external("assets",    m.assets,     Asset::Types    {  Asset::Color,     Asset::Displace, Asset::Normal });
        external("attr",      m.attr,       Vertex::Attribs { Vertex::Position, Vertex::UV,      Vertex::Normal });
        external("render",    m.render,     Rendering       { Rendering::Shader });
    }
    
    /// change management
    void changed(PropList list) {
        if (!list.count("model"))
            return;
        pipes = model<Vertex>(m.model, m.skin, m.ubo, m.attr, m.assets, m.shaders);
    }
    
    /// rendition of pipes
    Element render() {
        if (pipes) for (auto &[name, pipe]: pipes.map())
            device().render.push(pipe);
        return node::render();
    }
};
