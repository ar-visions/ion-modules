#pragma once

/// use of the meta struct is better, and doesnt need to register each time if that can be figured out that is
/// it can be done better.. with factory creation of its data output. thats compound/recursive
/// i want to essentially pack the data in as tight as possible, or aligned a certain amount
struct Vertex2: Struct<Vertex2> {
    M<vec3f> pos;
    M<vec3f> norm;
    M<vec2f> uv;
    M<vec4f> clr;
    ///
    void bind() {
        member <vec3f> ("pos",  pos );
        member <vec3f> ("norm", norm);
        member <vec2f> ("uv",   uv  );
        member <vec4f> ("clr",  clr );
    }
    ///
    struct_shim(Vertex2);
};

/// vulkan people like to flush the entire glyph cache per type
typedef array<VkVertexInputAttributeDescription> VAttribs;

///
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
    glm::vec4 clr;

    static VAttribs attribs() {
        return VAttribs {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  pos) }, /// data is inline
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, norm) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex,   uv) },
            { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex,  clr) }
        };
    }
    
    Vertex(vec3 pos, vec3 norm, vec2 uv, vec4 clr):
           pos  ({  pos.x,  pos.y,  pos.z        }),
           norm ({ norm.x, norm.y, norm.z        }),
           uv   ({   uv.x,   uv.y                }),
           clr  ({  clr.x,  clr.y,  clr.z, clr.w }) { }
    Vertex(vec3 &pos, vec3 &norm, vec2 &uv):
           pos({pos.x,pos.y,pos.z}), norm({norm.x,norm.y,norm.z}), uv({uv.x,uv.y}) { }
    
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
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
    VertexData(std::nullptr_t n = null) { }
    
    /// these things blow, so make everything based on initializer list with variable entry via [key]: value, also you can just order params in, if you dont specify key:
    /// C++ sucks with initializer list in that it didnt take over the language.  it should have.
    /// it could have removed much complexity and nonsense with this move.
    ///
    VertexData(Device &device, Buffer buffer, std::function<VAttribs()> fn_attribs) : device(&device), buffer(buffer), fn_attribs(fn_attribs)  { }
    
    operator bool () { return device != null; }
    bool operator!() { return device == null; }
};

///
template <typename V>
struct VertexBuffer:VertexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds,
                 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(std::nullptr_t n = null) : VertexData(n) { }
    VertexBuffer(Device &device, array<V> &v) : VertexData(device, Buffer {
            &device, v, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
            []() { return V::attribs(); }) { }
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
    string &operator[](string n) {
        return map[n];
    }
    size_t count(string n) {
        return map.count(n);
    }
};
