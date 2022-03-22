#pragma once

/// vulkan people like to flush the entire glyph cache per type
typedef array<VkVertexInputAttributeDescription> VAttribs;

/// soon: graph-mode shaders? not exactly worth the trouble at the moment especially with live updates not being as quick as glsl recompile
struct Vertex {
    enum Attr {
        Position  = (1 << 0),
        Normal    = (1 << 1),
        UV        = (1 << 2),
        Color     = (1 << 3),
        Tangent   = (1 << 4),
        BiTangent = (1 << 5),
    };
    
    ///
    typedef FlagsOf<Attr> Attribs;
    
    ///
    glm::vec3 pos;  // position
    glm::vec3 norm; // normal position
    glm::vec2 uv;   // texture uv coordinate
    glm::vec4 clr;  // color
    glm::vec3 ta;   // tangent
    glm::vec3 bt;   // bi-tangent, we need an arb in here, or two. or ten.
    
    ///
    static VAttribs attribs(FlagsOf<Attr> attr) {
        auto   res = VAttribs(6);
        if (attr[Position])  res += { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  pos)  };
        if (attr[Normal])    res += { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  norm) };
        if (attr[UV])        res += { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex,  uv)   };
        if (attr[Color])     res += { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex,  clr)  };
        if (attr[Tangent])   res += { 4, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  ta)   };
        if (attr[BiTangent]) res += { 5, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  bt)   };
        return res;
    }
    
    ///
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
    Device         *device = null;
    Buffer          buffer;
    operator VkBuffer() { return buffer; }
    size_t       size() { return buffer.sz / buffer.type_size; }
    IndexData(std::nullptr_t n = null) { }
    IndexData(Device &device, Buffer buffer) : device(&device), buffer(buffer)  { }
    operator bool () { return device != null; }
    bool operator!() { return device == null; }
};

///
struct VertexData {
    Device         *device = null;
    Buffer          buffer;
    std::function<VAttribs()> fn_attribs;
    ///
    VertexData(std::nullptr_t n = null) { }
    VertexData(Device &device, Buffer buffer, std::function<VAttribs()> fn_attribs) :
                device(&device), buffer(buffer), fn_attribs(fn_attribs)  { }
    ///
    operator VkBuffer() { return buffer;         }
    size_t       size() { return buffer.sz;      }
       operator bool () { return device != null; }
       bool operator!() { return device == null; }
};

///
template <typename V>
struct VertexBuffer:VertexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(std::nullptr_t n = null) : VertexData(n) { }
    VertexBuffer(Device &device, array<V> &v, Vertex::Attribs &attr) : VertexData(device, Buffer {
            &device, v, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
            [attr=attr]() { return V::attribs(attr); }) { }
    size_t size() { return buffer.sz / sizeof(V); }
};

///
template <typename I>
struct IndexBuffer:IndexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    IndexBuffer(std::nullptr_t n = null) : IndexData(n) { }
    IndexBuffer(Device &device, array<I> &i) : IndexData(device, Buffer {
            &device, i, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }) { }
};

///
struct Shaders {
    map<string, string> map;
    /// default construction
    Shaders(std::nullptr_t n = null) {
        map["*"] = "main";
    }
    
    bool operator==(Shaders &ref) {
        return map == ref.map;
    }
    
    operator bool()  { return  map.size(); }
    bool operator!() { return !map.size(); }
    
    /// group=shader
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
    string operator()(string &group) {
        return map.count(group) ? map[group] : map["*"];
    }
    string &operator[](string n) {
        return map[n];
    }
    size_t count(string n) {
        return map.count(n);
    }
};
