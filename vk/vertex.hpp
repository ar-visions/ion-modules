#pragma once

/// soon: graph-mode shaders? not exactly worth the trouble at the moment especially with live updates not being as quick as glsl recompile
struct Vertex {
    enum Attr {
        Position  = 0, /// same 'location' as glsl (with hope, that is)
        Normal    = 1,
        UV        = 2,
        Color     = 3,
        Tangent   = 4,
        BiTangent = 5,
    };
    
    ///
    typedef FlagsOf<Attr> Attribs;
    
    ///
    vec3 pos;  // position
    vec3 norm; // normal position
    vec2 uv;   // texture uv coordinate
    vec4 clr;  // color
    vec3 ta;   // tangent
    vec3 bt;   // bi-tangent, we need an arb in here, or two. or ten.
    
    ///
    static VAttribs attribs(FlagsOf<Attr> attr);
    
    ///
    Vertex(nullptr_t n = null) { }
    Vertex(vec3 pos, vec3 norm, vec2 uv, vec4 clr, vec3 ta = {}, vec3 bt = {}):
           pos  ({  pos.x,  pos.y,  pos.z        }),
           norm ({ norm.x, norm.y, norm.z        }),
           uv   ({   uv.x,   uv.y                }),
           clr  ({  clr.x,  clr.y,  clr.z, clr.w }),
           ta   ({   ta.x,   ta.y,   ta.z        }),
           bt   ({   bt.x,   bt.y,   bt.z        }) { }
    
    ///
    Vertex(vec3 &pos, vec3 &norm, vec2 &uv, vec4 &clr, vec3 &ta, vec3 &bt):
           pos  ({  pos.x,  pos.y,  pos.z        }),
           norm ({ norm.x, norm.y, norm.z        }),
           uv   ({   uv.x,   uv.y                }),
           clr  ({  clr.x,  clr.y,  clr.z, clr.w }),
           ta   ({   ta.x,   ta.y,   ta.z        }),
           bt   ({   bt.x,   bt.y,   bt.z        }) { }
    
    ///
    static array<Vertex> square(rgba clr = {1.0, 1.0, 1.0, 1.0}) {
        return array<Vertex> {
            Vertex {{-0.5, -0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}, clr},
            Vertex {{ 0.5, -0.5, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}, clr},
            Vertex {{ 0.5,  0.5, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0}, clr},
            Vertex {{-0.5,  0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}, clr}
        };
    }
};

///
struct IndexData {
    Device         *dev = null;
    Buffer          buffer;
    ///
    operator VkBuffer() { return buffer; }
    size_t       size() { return buffer.size(); }
    operator    bool () { return dev != null; }
    bool    operator!() { return dev == null; }
    ///
    IndexData(std::nullptr_t n = null) { }
    IndexData(Device &dev, Buffer buffer) : dev(&dev), buffer(buffer)  { }
};

struct VertexData {
    Device                   *dev = null;
    Buffer                    buffer;
    std::function<VAttribs()> fn_attribs;
    sh<VkWriteDescriptorSet>  wds;
    ///
    VkWriteDescriptorSet &operator()(VkDescriptorSet &ds);
    ///
    VertexData(std::nullptr_t n = null);
    VertexData(Device &dev, Buffer buffer, std::function<VAttribs()> fn_attribs);
    ///
    operator  VkBuffer &();
    size_t         size ();
       operator    bool ();
       bool    operator!();
    ///
    VertexData &operator=(const VertexData &vref);
};

/// deprecate in favor of composed vertex approach.
template <typename V>
struct VertexBuffer:VertexData {
    VertexBuffer(std::nullptr_t n = null) : VertexData(n) { }
    VertexBuffer(Device &dev, array<V> &v, Vertex::Attribs &attr) : VertexData(dev, Buffer {
            &dev, v.data(), Id<V>(), v.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
            [attr=attr]() { return V::attribs(attr); }) { }
    size_t size() { return buffer.size; }
};

///
template <typename I>
struct IndexBuffer:IndexData {
    IndexBuffer(std::nullptr_t n = null) : IndexData(n) { }
    IndexBuffer(Device &dev, array<I> &i) : IndexData(dev, Buffer {
            &dev, i.data(), Id<I>(), i.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }) { }
};

///
struct Shaders {
    map<string, string> map;
    /// default construction
    Shaders(std::nullptr_t n = null) { map["*"] = "main"; }
    
    /// *=shader3,group=shader,group2=shader2
    Shaders(string v) { /// str is the only interface in it.  everything else is just too messy for how simple the map is
        auto sp = v.split(",");
        for (auto v: sp) {
            auto a = v.split("=");
            assert(a.size() == 2);
            string key   = a[0];
            string value = a[1];
            map[key] = value;
        }
    }
    Shaders(const char * v) : Shaders(str(v)) { }
    bool     operator== (Shaders &ref) { return map == ref.map;   }
    operator      bool()               { return  map.size();   }
    bool     operator!()               { return !map.size();   }
    str      operator ()(str &group)   { return  map.count(group) ? map[group] : map["*"]; }
    str &    operator[] (str n)        { return  map[n];       }
    size_t      count   (str n)        { return  map.count(n); }
};
