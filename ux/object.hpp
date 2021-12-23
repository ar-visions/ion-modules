#pragma once
#include <ux/ux.hpp>

template <typename V>
struct Object:node {
    declare(Object);
    
    struct Members {
        Extern<path_t>      model;
        Extern<var>         shaders;
        Extern<UniformData> ubo;
        Extern<vec<Attrib>> attr;
        /// --------------------- cross the streams?
        Intern<str>         sval;
        Intern<int>         ivar;
        Intern<PipelineMap> pmap;
    } m;

    void binds() {
        external("uniform", m.ubo,       UniformData { null });
        external("model",   m.model,     null);
        external("shaders", m.shaders,   Args {{"*", "main"}});
        external("attr",    m.attr,      vec<Attrib> { Position3f() });
        /// ---------------------
        internal("sval",    m.sval,      "");
        internal("ivar",    m.ivar,      0);
        internal("pmap",    m.pmap,      PipelineMap { null }); /// call it
    }
    
    // changed method.
    // perform node updates, thats not very hard to do.
    
    void changed(PropList list) {
        if (list.count("model") == 0)
            return;
        
        path_t &mod = m.model;
        if (std::filesystem::exists(mod)) {
            auto shaders = ShaderMap { };
            var &sh = m.shaders;
            for (auto &[group, shader]: sh.map())
                shaders[group] = shader;
            /// --------------------------------------------------
            assert(m.ubo);
            m.pmap = model<Vertex>(mod, m.ubo, m.attr, shaders);
        } else {
            //pmap = PipelineMap { null };
        }
    }
    
    Element render() {
        /*
        auto &device = Vulkan::device();
        for (auto &[group, pipeline]: m)
            device.render.push(pipeline);
        */
        return null;
    }
};

