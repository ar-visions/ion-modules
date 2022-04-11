#pragma once
#include <vk/device.hpp>
#include <vk/vk.hpp>
#include <vk/opaque.hpp>
#include <dx/map.hpp>
#include <dx/array.hpp>

struct vkState;
typedef std::function<void(vkState &)> VkStateFn;

/// explicit use of the implicit constructor with this comment
struct iPipelineData;
struct  PipelineData {
    operator bool  ();
    bool operator! ();
    void   enable  (bool en);
    bool operator==(PipelineData &b);
    void   update  (size_t frame_id);
    ///
    PipelineData(nullptr_t = null);
    PipelineData(Device &device, UniformData &ubo,   VertexData &vbo,    IndexData &ibo,
                 Assets &assets, size_t       vsize, string      shader, VkStateFn  vk_state = null);
    ///
    sh<iPipelineData> intern;
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device    &dev, UniformData &ubo,     VertexData &vbo,
             IndexData &ibo,    Assets   &assets,  string      name):
        PipelineData(dev, ubo, vbo, ibo, assets, sizeof(V), name) { }
};

/// These are calling for Daniel!
struct Pipes {
    struct Data {
        Device       *dev     = null;
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
    Model(Device &dev) {
        data = std::shared_ptr<Data>(new Data { &dev });
    }
    
    static str form_path(str base, str model, str skin, str asset, str ext) {
        return fmt {"{0}/{1}{2}{3}{4}", {
            base, model, skin  ? str("." + skin)  : str(""),
                         asset ? str("." + asset) : str(""), ext}};
    }
    
    /// optional skin arg
    Assets cache_assets(str model, str skin, Asset::Types &atypes) {
        auto              &d = *this->data;
        ResourceCache &cache = data->dev->get_cache(); /// load all cash and ass hats
        Assets        assets = Assets(Asset::Max);
        ///
        auto load_tx = [&](Asset::Type t, Path p) -> Texture * {
            if (!cache.count(p)) {
                cache[p].image     = new Image(p, Image::Rgba);
                auto          &shh = cache[p].image->pixels;
                ::Shape<Major> sh1 = shh.shape();
                /// bookmark: reload texture on pipeline refresh
                cache[p].texture  = new Texture(d.dev, *cache[p].image, t);
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
    
    Model(str name, str skin, Device &dev, UniformData &ubo, Vertex::Attribs &attr,
          Asset::Types &atypes, Shaders &shaders) : Model(dev)
    {
        /// cache control for images to texture here; they might need a new reference upon pipeline rebuild
        auto assets = cache_assets(name, skin, atypes);
        auto  mpath = form_path("models", name, null, null, ".obj");
        
        /// load .obj file
        auto    obj = Obj<V>(mpath, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        
        auto &d = *this->data;
        VertexData vd = VertexBuffer<V>(dev, obj.vbo, attr);
        d.vbo   = vd;
        for (auto &[name, group]: obj.groups) {
            if (!group.ibo)
                continue;
            auto     ibo = IndexBuffer<uint32_t>(dev, group.ibo);
            PipelineData pd = Pipeline<V>(dev, ubo, d.vbo, ibo, assets, shaders(name));
            d.part[name] = pd;
        }
    }
};
