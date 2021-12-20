#pragma once
#include <ux/ux.hpp>

template <typename V>
struct Object:node {
    declare(Object);
    enum Uniform { U_MVP };
    PipelineMap m;
    
    struct Props:IProps {
        str         model;
        var         shaders;
        UniformData ubo;
        vec<Attrib> attr;
    } props;
    
    void define() {
        //Define <UniformData> { this, "uniform", &props.ubo,      UniformData {null}       };
        //Define <str>         { this, "model",   &props.model,   "this had better change." };
        //Define <var>         { this, "shaders", &props.shaders,  Args {{"*", "main"}}     };
        //ArrayOf <Attrib>     { this, "attr",    &props.attr,     vec<Attrib> { Position3f() }};
    }
    
    void changed(PropList list) {
        if (props.model) {
            auto shaders = ShaderMap { };
            for (auto &[group,   shader]: props.shaders.map())
                shaders[group] = shader;
            ///
            assert(props.ubo);
            m = model<Vertex>(props.model, props.ubo, props.attr, shaders);
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

