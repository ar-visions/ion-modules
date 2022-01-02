#pragma once
#include <ux/style.hpp>
#include <ux/node.hpp>
//#include <ux/ux.hpp>

template <typename V>
struct Object:node {
    declare(Object);
    
    struct Members {
        Extern<path_t>      model;
        Extern<Shaders>     shaders;
        Extern<UniformData> ubo;
        Extern<vec<Attrib>> attr;
        /// --------------------- cross the streams?
        Intern<str>         sval;
        Intern<int>         ivar;
        Intern<PipelineMap> pmap;
    } m;

    void bind() {
        external("uniform", m.ubo,       UniformData { null });
        external("model",   m.model,     path_t      {  });
        external("shaders", m.shaders,   Shaders     {"*=main"});
        external("attr",    m.attr,      vec<Attrib> { Position3f() });
        /// ----------------------------------------------------------
        internal("sval",    m.sval,      str         { "" });
        internal("ivar",    m.ivar,      int         { 0 });
        internal("pmap",    m.pmap,      PipelineMap { null });
    }
    
    void changed(PropList list) {
        if (list.count("model") == 0)
            return;
        /// ------------------------
        path_t &mod = m.model;
        m.pmap = exists(mod) ?
            model<Vertex>(mod, m.ubo, m.attr, m.shaders) :
            PipelineMap { null };
    }
    
    Element render() { /// no reference returned, so it wont do a thing except push this pipeline
        auto &device = Vulkan::device();
        for (auto &[group, pipeline]: m)
            device.render.push(pipeline);
        return null;
    }
};

