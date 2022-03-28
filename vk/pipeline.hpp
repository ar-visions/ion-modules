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
        Assets                     assets;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        map<Path, Texture *>       tx_cache;
        size_t                     vsize;
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
               Assets &assets, size_t vsize, string name, VkStateFn vk_state);
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
    PipelineData(Device &device, UniformData &ubo,   VertexData &vbo,    IndexData &ibo,
                 Assets &assets, size_t       vsize, string      shader, VkStateFn  vk_state = null) {
            m = std::shared_ptr<Memory>(new Memory { device, ubo, vbo, ibo, assets, vsize, shader, vk_state });
        }
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device    &device, UniformData &ubo,     VertexData &vbo,
             IndexData &ibo,    Assets      &assets,  string      name):
        PipelineData(device, ubo, vbo, ibo, assets, sizeof(V), name) { }
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
    
    static str form_path(str base, str model, str skin, str asset, str ext) {
        return fmt {"{0}/{1}{2}{3}{4}", {
            base, model, skin  ? str("." + skin)  : str(""),
                         asset ? str("." + asset) : str(""), ext}};
    }
    
    /// optional skin arg
    Assets cache_assets(str model, str skin, Asset::Types &atypes) {
        auto   &d     = *this->data;
        auto   &cache = d.device->tx_cache; /// ::map<Path, Device::Pair>
        Assets assets = Assets(Asset::Max);
        ///
        auto load_tx = [&](Asset::Type t, Path p) -> Texture * {
            if (!cache.count(p)) {
                cache[p].image     = new Image(p, Image::Rgba);
                auto          &shh = cache[p].image->pixels;
                ::Shape<Major> sh1 = shh.shape();
                /// bookmark: reload texture on pipeline refresh
                cache[p].texture  = new Texture(d.device, *cache[p].image, t);
                cache[p].texture->push_stage(Texture::Stage::Shader);
            };
            return cache[p].texture;
        };
        
        ///
        static auto names = ::map<Asset::Type, str> {
            { Asset::Color,    "color"    },
            { Asset::Normal,   "normal"   },
            { Asset::Mask,     "mask"     },
            { Asset::Shine,    "shine"    },
            { Asset::Rough,    "rough"    },
            { Asset::Displace, "displace" }
        };
        
        /// for each
        for (auto &[type, n]:names) {
            if (!atypes[type])
                continue;

            /// prefer png over jpg, if either one exists
            Path png = form_path("textures", model, skin, n, ".png");
            Path jpg = form_path("textures", model, skin, n, ".jpg");
            console.assertion(png.exists() || jpg.exists(), "image does not exist: {0}", {png});
            ///
            assets[type] = load_tx(type, png.exists() ? png : jpg);
            /// atm texture doesnt know what sort of asset type it is,
            /// and it perhaps could be used to simplify params but
            /// thats just organization to be done later [todo]
        }
        return assets;
    }
    
    Model(str name, str skin, Device &device, UniformData &ubo, Vertex::Attribs &attr,
          Asset::Types &atypes, Shaders &shaders) : Model(device)
    {
        /// cache control for images to texture here; they might need a new reference upon pipeline rebuild
        auto assets = cache_assets(name, skin, atypes);
        auto  mpath = form_path("models", name, null, null, ".obj");
        
        /// load .obj file
        auto    obj = Obj<V>(mpath, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        
        auto &d = *this->data;
        d.vbo   = VertexBuffer<V>(device, obj.vbo, attr);
        for (auto &[name, group]: obj.groups) {
            if (!group.ibo)
                continue;
            auto     ibo = IndexBuffer<uint32_t>(device, group.ibo);
            d.part[name] = Pipeline<V>(device, ubo, d.vbo, ibo, assets, shaders(name));
        }
    }
};
