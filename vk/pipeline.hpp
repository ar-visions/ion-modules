#pragma once
#include <vk/device.hpp>
#include <vk/vk.hpp>

struct Attrib {
    enum Type {
        Position3f,
        Normal3f,
        Texture2f,
        Color4f,
    };
    Type     type;
    VkFormat format;
    Texture  tx;
    Attrib(Type t, VkFormat f, Texture tx): type(t), format(f), tx(tx) { }
    
    static vec<VkVertexInputAttributeDescription> vk(uint32_t binding, vec<Attrib> &ax) {
        uint32_t location = 0;
        uint32_t offset   = 0;
        auto     result   = vec<VkVertexInputAttributeDescription>(ax.size());
        for (auto &a: ax) {
            result += VkVertexInputAttributeDescription {
                .binding  = binding,
                .location = location++,
                .format   = a.format,
                .offset   = offset
            };
            switch (a.type) {
                case Attrib::Position3f: offset += sizeof(vec3f); break;
                case Attrib::Normal3f:   offset += sizeof(vec3f); break;
                case Attrib::Texture2f:  offset += sizeof(vec2f); break;
                case Attrib::Color4f:    offset += sizeof(vec4f); break;
            }
        }
        return result;
    }
};

struct Position3f : Attrib { Position3f()           : Attrib(Attrib::Position3f, VK_FORMAT_R32G32B32_SFLOAT,    null) { }};
struct Normal3f   : Attrib { Normal3f  ()           : Attrib(Attrib::Normal3f,   VK_FORMAT_R32G32B32_SFLOAT,    null) { }};
struct Texture2f  : Attrib { Texture2f (Texture tx) : Attrib(Attrib::Texture2f,  VK_FORMAT_R32G32_SFLOAT,       tx)   { }};
struct Color4f    : Attrib { Color4f   ()           : Attrib(Attrib::Color4f,    VK_FORMAT_R32G32B32A32_SFLOAT, null) { }};

///
/// makes a bit of sense that Model is the interface into Vertex, its Attribs and Pipeline, correct?
///
/// explicit use of the implicit constructor with this comment
struct PipelineData {
    struct Memory {
        Device                    *device;
        std::string                shader;          /// name of the shader.  worth saying.
        vec<VkCommandBuffer>       frame_commands;  /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
        vec<VkDescriptorSet>       desc_sets;       /// needs to be in frame, i think.
        VkPipelineLayout           pipeline_layout; /// and what a layout
        VkPipeline                 pipeline;        /// pipeline handle
        UniformData                ubo;             /// we must broadcast this buffer across to all of teh swap uniforms
        VertexData                 vbo;             ///
        IndexData                  ibo;
        vec<VkVertexInputAttributeDescription> attr;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        map<std::string, Texture>  tx;
        size_t                     vsize;
        int                        sync = -1;
        operator bool ();
        bool operator!();
        void enable (bool en);
        void update(size_t frame_id);
        Memory(nullptr_t    n = null);
        Memory(Device      &device,
               UniformData &ubo,
               VertexData  &vbo,
               IndexData   &ibo,
               vec<Attrib> &attr, size_t vsize, std::string name);
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
                 vec<Attrib> attr,
                 size_t vsize, std::string shader) {
            m = std::shared_ptr<Memory>(
                new Memory { device, ubo, vbo, ibo, attr, vsize, shader }
            );
        }
    /// general query engine for view construction and model definition to view creation
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
             vec<Attrib> attr, std::string name):
        PipelineData(device, ubo, vbo, ibo, attr, sizeof(V), name) { }
};

/// eventually different textures for the different parts..
struct PipelineMap {
    struct Data {
        Device      *device  = null;
        VertexData   vbo     = null;
        uint32_t     binding = 0;
        vec<Attrib>  attr    = {};
        map<str, PipelineData> part;
    };
    // beautiful.
    // # think about use for ux here, can very well be part of the define()
    std::shared_ptr<Data> data;
};

///
/// model dx (using pipeline dx) -- best gaia can summon in me
template <typename V>
struct Model:PipelineMap {
    Model(Device &device, UniformData &ubo, vec<Attrib> ax, std::filesystem::path p) {
        this->data = std::shared_ptr<Data>(new Data { &device });
        auto &data = *this->data;
        /// every group is essentially pipeline town going into a map (vim)
        auto   obj = Obj<V>(p, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        data.vbo = VertexBuffer<V>(device, obj.vbo);
        for (auto &[name, group]: obj.groups) {
            auto        ibo = IndexBuffer<uint32_t>(group.ibo);
            data.part[name] = Pipeline<V>(device, ubo, data.vbo, ibo, ax, name);
        }
    }
};


