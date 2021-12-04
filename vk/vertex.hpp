#pragma once

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 uv;
    static vec<VkVertexInputAttributeDescription> attrs();
};

struct IndexData {
    Device         *device = null;
    Buffer          buffer;
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
    IndexData(Device &device, Buffer buffer) : device(&device), buffer(buffer)  { }
    operator bool () { return device != null; }
    bool operator!() { return device == null; }
};

struct VertexData {
    Device         *device = null;
    Buffer          buffer;
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
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
    VertexBuffer(Device &device, vec<V> &v) : VertexData(device, Buffer {
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
    IndexBuffer(Device &device, vec<I> &i) : IndexData(device, Buffer {
            &device, i, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }) { }
    size_t size() { return buffer.sz / sizeof(I); }
};
