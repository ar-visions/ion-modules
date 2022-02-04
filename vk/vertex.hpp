#pragma once

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
    glm::vec4 clr;
    static array<VkVertexInputAttributeDescription> attrs();
    Vertex(vec3 pos, vec3 norm, vec2 uv, vec4 clr):
           pos({pos.x,pos.y,pos.z}),
           norm({norm.x,norm.y,norm.z}),
           uv({uv.x,uv.y}),
           clr({clr.x,clr.y,clr.z,clr.w}) { }
    Vertex(vec3 &pos, vec3 &norm, vec2 &uv):
           pos({pos.x,pos.y,pos.z}), norm({norm.x,norm.y,norm.z}), uv({uv.x,uv.y}) { }
    
    /// very special function.
    static array<Vertex> square(rgba clr = {1.0, 1.0, 1.0, 1.0}) {
        return array<Vertex> {
            Vertex {{-0.5, -0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}, clr},
            Vertex {{ 0.5, -0.5, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}, clr},
            Vertex {{ 0.5,  0.5, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0}, clr},
            Vertex {{-0.5,  0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}, clr}
        };
    }
};

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

struct VertexData {
    Device         *device = null;
    Buffer          buffer;
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
    VertexData(std::nullptr_t n = null) { }
    VertexData(Device &device, Buffer buffer) : device(&device), buffer(buffer)  { }
    operator bool () { return device != null; }
    bool operator!() { return device == null; }
};

template <typename V>
struct VertexBuffer:VertexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(std::nullptr_t n = null) : VertexData(n) { }
    VertexBuffer(Device &device, array<V> &v) : VertexData(device, Buffer {
            &device, v, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }) { }
    size_t size() { return buffer.sz / sizeof(V); }
};

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

struct Shaders {
    map<std::string, std::string> map;
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
    Shaders(str v) { /// str is the only interface in it.  everything else is just too messy for how simple the map is
        auto sp = v.split(",");
        for (auto v: sp) {
            auto a = v.split("=");
            assert(a.size() == 2);
            std::string key   = a[0];
            std::string value = a[1];
            map[key] = value;
        }
    }
    std::string &operator[](std::string n) {
        return map[n];
    }
    size_t count(std::string n) {
        return map.count(n);
    }
};



