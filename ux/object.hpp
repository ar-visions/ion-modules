#pragma once
#include <ux/ux.hpp>

struct Object:node {
    declare(Object);
    enum Uniform { U_MVP };
    PipelineMap pmap;
    
    struct Props:IProps {
        str         model;
        var         shaders;
        UniformData ubo;
    } props;
    
    void define() {
        Define <str>         { this, "model",   &props.model,   "this had better change." };
        Define <var>         { this, "shaders", &props.shaders,  Args {{"*", "main"}}     };
        Define <UniformData> { this, "uniform", &props.ubo,      UniformData {null}       };
    }
    
    void changed(PropList list) {
        if (props.model) {
            auto shaders = ShaderMap { };
            for (auto &[group,   shader]: props.shaders.map())
                shaders[group] = shader;
            ///
            assert(props.ubo);
            pmap = obj<Vertex>(props.ubo, props.model, shaders);
        } else {
            pmap = PipelineMap { null };
        }
    }
    
    Element render() {
        /// hit 88mph...
        auto &device = Vulkan::device();
        for (auto &[group, pipeline]: pmap.map())
            device.render.push(pipeline);
        /// and gone...
        return null;
    }
};

