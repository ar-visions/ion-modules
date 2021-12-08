#pragma once
#include <vk/device.hpp>

/// explicit use of the implicit constructor with this comment
struct PipelineData {
    struct Memory {
        Device                    *device;
        std::string                name;
        vec<VkCommandBuffer>       frame_commands;  /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
        vec<VkDescriptorSet>       desc_sets;       /// needs to be in frame, i think.
        VkPipelineLayout           pipeline_layout; /// and what a layout
        VkPipeline                 pipeline;        /// pipeline handle
        UniformData                ubo;             /// we must broadcast this buffer across to all of teh swap uniforms
        VertexData                 vbo;             ///
        IndexData                  ibo;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        vec<VkVertexInputAttributeDescription> attrs;
        map<std::string, Texture>  tx;
        size_t                     vsize;
        int                        sync = -1;
        operator bool ();
        bool operator!();
        void enable (bool en);
        void update(size_t frame_id);
        Memory(nullptr_t n = null);
        Memory(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                     vec<VkVertexInputAttributeDescription> attrs, size_t vsize, std::string name);
        void destroy();
        void initialize();
        ~Memory();
    };
    std::shared_ptr<Memory> m;
    ///
    operator bool  ()                { return  m->enabled; }
    bool operator! ()                { return !m->enabled; }
    void enable    (bool en)         { m->enabled = en; }
    bool operator==(PipelineData &b) { return m == b.m; }
    PipelineData(nullptr_t n = null) { }
    void update(size_t frame_id);
    ///
    PipelineData(Device &device,
                 UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 vec<VkVertexInputAttributeDescription> attrs,
                 size_t vsize, std::string name) {
            /// allocate and set memory
            m = std::shared_ptr<Memory>(
                new Memory { device, ubo, vbo, ibo, attrs, vsize, name }
            );
        }
    /// general query engine for view construction and model definition to view creation
};

/// we need high level abstraction to get to a vulkanized component model
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo, std::string name):
            PipelineData(device, ubo, vbo, ibo, V::attrs(), sizeof(V), name) { }
};

/// change this damn name. haha.
struct PipelineMap {
    struct Data {
        map<std::string, PipelineData> groups;
    };
    std::shared_ptr<Data> data;
    PipelineData &operator[](std::string name) {
        return data->groups[name];
    }
    size_t  count (std::string n) { return data ? data->groups.count(n) : 0; }
    size_t   size () { return data ? data->groups.size() : 0; }
    bool operator!() { return data == null; }
    operator bool () { return data != null; }
    map<std::string, PipelineData> &map() {
        return data->groups;
    }
    PipelineMap(nullptr_t n = null) : data(null) { }
    PipelineMap(size_t    a)        : data(a ? std::shared_ptr<Data>(new Data { a }) : null) { }
};

