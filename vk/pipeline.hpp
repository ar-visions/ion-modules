#pragma once
#include <vk/device.hpp>
#include <vk/vk.hpp>
#include <dx/map.hpp>
#include <dx/array.hpp>

/// name better, ...this.
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
        array<VkCommandBuffer>       frame_commands;  /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
        array<VkDescriptorSet>       desc_sets;       /// needs to be in frame, i think.
        VkPipelineLayout           pipeline_layout; /// and what a layout
        VkPipeline                 pipeline;        /// pipeline handle
        UniformData                ubo;             /// we must broadcast this buffer across to all of the swap uniforms
        VertexData                 vbo;             ///
        IndexData                  ibo;
        // array<VkVertexInputAttributeDescription> attr; -- vector is given via VertexData::fn_attribs(). i dont believe those need to be pushed up the call chain
        array<Texture *>           tx;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        map<Path, Texture *>       tx_cache;
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
        Memory(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
               array<Texture *> &tx, size_t vsize, rgba clr, string name, VkStateFn vk_state);
        void destroy();
        void initialize();
        ~Memory();
    };
    std::shared_ptr<Memory> m;
    ///
    operator bool  ()                { return  m->enabled; }
    bool operator! ()                { return !m->enabled; }
    void   enable  (bool en)         { m->enabled = en; }
    bool operator==(PipelineData &b) { return m == b.m; }
    PipelineData   (std::nullptr_t n = null) { }
    void   update  (size_t frame_id);
    ///
    PipelineData(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 array<Texture *> &tx, size_t vsize, rgba clr, string shader, VkStateFn vk_state = null) {
            m = std::shared_ptr<Memory>(new Memory { device, ubo, vbo, ibo, tx, vsize, clr, shader, vk_state });
        }
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo, array<Texture *> tx, rgba clr, string name):
        PipelineData(device, ubo, vbo, ibo, tx, sizeof(V), clr, name) { }
};

/// these are calling for Daniel
struct Pipes {
    struct Data {
        Device       *device  = null;
        VertexData    vbo     = null;
        uint32_t      binding = 0;
      //array<Attrib> attr    = {};
        map<str, PipelineData> part;
    };
    std::shared_ptr<Data> data;
    operator bool() { return !!data; }
    inline map<str, PipelineData> &map() { return data->part; }
};

/// Model is the integration of device, ubo, attrib, with interfaces around OBJ format or Lambda
template <typename V>
struct Model:Pipes {
    struct Polys {
        ::map<str, array<int32_t>> groups;
        array<V> verts;
    };
    
    ///
    struct Shape {
        typedef std::function<Polys(void)> ModelFn;
        ModelFn fn;
    };
    
    ///
    Model(Device &device) {
        data = std::shared_ptr<Data>(new Data { &device });
    }
    
    ///
    array<Texture*> cache_textures(array<Path> &images) {
        auto &d = *this->data;
        ::map<Path, Device::Pair> &cache = d.device->tx_cache;
        array<Texture*> tx = array<Texture*>(images.size());
        for (Path &p:images) {
            if (!cache.count(p)) {
                cache[p].image = new Image(p, Image::Rgba);
                auto          &shh = cache[p].image->pixels;
                ::Shape<Major> sh1 = shh.shape();
                /// bookmark: reload texture on pipeline refresh
                cache[p].texture  = new Texture(d.device, *cache[p].image,
                   VK_IMAGE_USAGE_SAMPLED_BIT      | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_R8G8B8A8_UNORM, -1);
                cache[p].texture->push_stage(Texture::Stage::Shader);
            }
            tx += cache[p].texture;
        }
        return tx;
    }
    
    ///
    Model(Device &device, UniformData &ubo, FlagsOf<Vertex::Attr> &attr, array<Path> &images, Path p, Shaders &shaders) : Model(device) {
        /// cache control for images to texture here; they might need a new reference upon pipeline rebuild
        auto  tx = cache_textures(images);
        auto obj = Obj<V>(p, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        auto &d = *this->data;
        d.vbo   = VertexBuffer<V>(device, obj.vbo, attr);
        for (auto &[name, group]: obj.groups) {
            auto     ibo = IndexBuffer<uint32_t>(device, group.ibo);
            d.part[name] = Pipeline<V>(device, ubo, d.vbo, ibo, tx, rgba {0.0, 0.0, 0.0, 0.0}, shaders(name));
        }
    }
    
    /// deprecare
    Model(Device &device, UniformData &ubo, array<Path> &images, Shape s, Shaders &shaders) : Model(device) {
        auto    &d = *this->data;
        auto polys = s.fn();
        auto    tx = cache_textures(images);
        d.vbo      = VertexBuffer<V>(device, polys.verts);
        for (auto &[name, group]: polys.groups) {
            auto     ibo = IndexBuffer<uint32_t>(device, group.ibo);
            d.part[name] = Pipeline<V>(device, ubo, d.vbo, ibo, tx, null, name);
        }
    }
};
