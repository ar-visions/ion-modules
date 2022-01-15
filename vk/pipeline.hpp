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
    // texture data needs to be a ref pass-through of the data, cast will suffice
    Attrib(var &v) {
        /// we can now load this from string, or var
        /// its not good enough to just throw the session info around, we need enough to create it.
        ///
    }
    operator var() {
        return Args {
            {"type", int(type)},
            {"tx",   VoidRef((void *)tx.get_data())}
        };
    }
    static vec<VkVertexInputAttributeDescription> vk(uint32_t binding, vec<Attrib> &ax) {
        uint32_t location = 0;
        uint32_t offset   = 0;
        auto     result   = vec<VkVertexInputAttributeDescription>(ax.size());
        for (auto &a: ax) {
            result += VkVertexInputAttributeDescription {
                .location = location++,
                .binding  = binding,
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

typedef vec<Attrib> VAttr;


/// just a solid, slab of state.  fresh out of the fires of Vulkan
/// Will be initialized by the general pipeline code, then optionally passed into lambda where it is altered
struct vkState {
    VkPipelineVertexInputStateCreateInfo    vertex_info {};
    VkPipelineInputAssemblyStateCreateInfo  topology    {};
    VkPipelineViewportStateCreateInfo       vs          {};
    VkPipelineRasterizationStateCreateInfo  rs          {};
    VkPipelineMultisampleStateCreateInfo    ms          {};
    VkPipelineDepthStencilStateCreateInfo   ds          {};
    VkPipelineColorBlendAttachmentState     cba         {};
    VkPipelineColorBlendStateCreateInfo     blending    {};
};

typedef std::function<void(vkState &)> VkStateFn;

/// explicit use of the implicit constructor with this comment
struct PipelineData {
    struct Memory {
        Device                    *device;
        std::string                shader;          /// name of the shader.  worth saying.
        vec<VkCommandBuffer>       frame_commands;  /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
        vec<VkDescriptorSet>       desc_sets;       /// needs to be in frame, i think.
        VkPipelineLayout           pipeline_layout; /// and what a layout
        VkPipeline                 pipeline;        /// pipeline handle
        UniformData                ubo;             /// we must broadcast this buffer across to all of the swap uniforms
        VertexData                 vbo;             ///
        IndexData                  ibo;
        //vec<VkVertexInputAttributeDescription> attr;
        vec<Attrib>                attr;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        map<std::string, Texture>  tx;
        size_t                     vsize;
        rgba                       clr;
        ///
        /// state vars
        int                        sync = -1;
        ///
        operator bool ();
        bool operator!();
        void enable (bool en);
        void update(size_t frame_id);
        Memory(std::nullptr_t    n = null);
        Memory(Device      &device,
               UniformData &ubo,
               VertexData  &vbo,
               IndexData   &ibo,
               vec<Attrib> &attr, size_t vsize, rgba clr, std::string name, VkStateFn vk_state);
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
    PipelineData(std::nullptr_t n = null) { }
    void update(size_t frame_id);
    ///
    PipelineData(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 vec<Attrib> attr, size_t vsize, rgba clr, std::string shader,
                 VkStateFn vk_state = null) {
            m = std::shared_ptr<Memory>(
                new Memory { device, ubo, vbo, ibo, attr, vsize, clr, shader, vk_state }
            );
        }
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
             vec<Attrib> attr, rgba clr, std::string name):
        PipelineData(device, ubo, vbo, ibo, attr, sizeof(V), clr, name) { }
};

/// these are calling for Daniel.
/// this is an ensemble of a lot of things, working together.
struct Pipes {
    struct Data {
        Device      *device  = null;
        VertexData   vbo     = null;
        uint32_t     binding = 0;
        //vec<Attrib>  attr    = {};
        map<str, PipelineData> part;
    };
    std::shared_ptr<Data> data;
    operator bool() { return !!data; }
    inline map<str, PipelineData> &map() { return data->part; }
};

/// model dx (using pipeline dx)
template <typename V>
struct Model:Pipes {
    Model(Device &device, UniformData &ubo, vec<Attrib> &ax, std::filesystem::path p) {\
        this->data = std::shared_ptr<Data>(new Data { &device });
        auto &data = *this->data;
        auto   obj = Obj<V>(p, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        ///
        data.vbo   = VertexBuffer<V>(device, obj.vbo);
        for (auto &[name, group]: obj.groups) {
            auto        ibo = IndexBuffer<uint32_t>(device, group.ibo);
            data.part[name] = Pipeline<V>(device, ubo, data.vbo, ibo, ax, null, name);
        }
    }
};


