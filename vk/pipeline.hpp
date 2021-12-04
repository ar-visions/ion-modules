#pragma once
#include <vk/device.hpp>

struct RenderPair {
    VkCommandBuffer   cmd; /// call them this, for ease of understanding of purpose
    UniformData       ubo; /// a uniform buffer object is always occupying a cmd of any sort when it comes to rendering
};

struct PipelineData {
    Device                    *device;
    std::string                name;
    vec<RenderPair>            pairs; /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
    vec<VkDescriptorSet>       desc_sets; /// needs to be in frame, i think.
    VkPipelineLayout           pipeline_layout;
    VkPipeline                 pipeline;
    UniformData                ubo;
    VertexData                 vbo;
    IndexData                  ibo;
    VkDescriptorSetLayout      set_layout;
    bool                       enabled;
    vec<VkVertexInputAttributeDescription> attrs;
    map<std::string, Texture>  tx;
    size_t                     vsize;
    ///
    operator bool () { return  enabled; }
    bool operator!() { return !enabled; }
    void enable (bool en) { enabled = en; }
    void update(Frame &frame, VkCommandBuffer &rc);
    PipelineData(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 vec<VkVertexInputAttributeDescription> attrs, size_t vsize, std::string name) :
        device(&device), name(name), ubo(ubo), vbo(vbo), ibo(ibo), attrs(attrs), vsize(vsize) {
            initialize();
        }
    virtual void destroy();
    virtual void initialize();
};

/// we need high level abstraction to get to a vulkanized component model
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo, std::string name):
            PipelineData(device, ubo, vbo, ibo, V::attrs(), sizeof(V), name) { }
};

