#pragma once

struct Device;
struct Texture;
struct Buffer {
    ///
    Device        *device;
    VkDescriptorBufferInfo info;
    VkBuffer       buffer;
    VkDeviceMemory memory;
    size_t         sz;
    size_t         type_size;
    void           destroy();
    operator       VkBuffer();
    operator       VkDeviceMemory();
    ///
    Buffer(nullptr_t n = null) : device(null) { }
    Buffer(Device *, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    
    template <typename T>
    Buffer(Device *d, vec<T> &v, VkBufferUsageFlags usage, VkMemoryPropertyFlags mprops): sz(v.size() * sizeof(T)) {
        VkDevice device = handle(d);
        VkBufferCreateInfo bi {};
        bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size         = VkDeviceSize(sz);
        bi.usage        = usage;
        bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

        assert(vkCreateBuffer(device, &bi, nullptr, &buffer) == VK_SUCCESS);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buffer, &req);
        
        /// allocate and bind
        VkMemoryAllocateInfo alloc {};
        alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize    = req.size;
        VkPhysicalDevice gpu    = gpu_handle(d);
        alloc.memoryTypeIndex   = memory_type(gpu, req.memoryTypeBits, mprops);
        assert(vkAllocateMemory(device, &alloc, nullptr, &memory) == VK_SUCCESS);
        vkBindBufferMemory(device, buffer, memory, 0);
        
        /// transfer memory
        assert(v.size() == sz / sizeof(T));
        void* data;
          vkMapMemory(device, memory, 0, sz, 0, &data);
               memcpy(data,   v.data(),  sz);
        vkUnmapMemory(device, memory);

        info        = VkDescriptorBufferInfo {};
        info.buffer = buffer;
        info.offset = 0;
        info.range  = sz; /// was:sizeof(UniformBufferObject)
    }
    
    void  copy_to(Buffer &, size_t);
    void  copy_to(Texture *);
    ///
    inline operator VkDescriptorBufferInfo &() {
        return info;
    }
};

struct IBufferData {
    Buffer          buffer;
    bool            short_index;
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
};

struct VBufferData {
    Buffer          buffer;
    operator VkBuffer() { return buffer;    }
    size_t       size() { return buffer.sz; }
};

/// its the stack, thats all.
template <typename V>
struct VertexBuffer:VBufferData {
    ///
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(VkDevice d, vec<V> &v) {
        buffer = {
            d, sizeof(V) * v.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    }
    size_t size() { return buffer.sz / sizeof(V); }
};

template <typename I>
struct IndexBuffer:IBufferData {
    ///
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    IndexBuffer(VkDevice d, vec<I> &i) {
        if constexpr (std::is_same_v<I, uint16_t>)
            short_index = true;
        else
            short_index = false;
        buffer = {
            d, sizeof(I) * i.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    }
    size_t size() { return buffer.sz / sizeof(I); }
};

struct UniformBufferData { /// UniformsData
    Device *device;
    Buffer  buffer;
    void   *data; /// its good to fix the address memory, update it as part of the renderer, if the user goes away the UniformBuffer will as well.
    
    UniformBufferData(Device *device = null) { }
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds);
    void destroy();
    void transfer();
    operator bool() {
        return device != null;
    }
    bool operator!() {
        return device == null;
    }
};

template <typename U>
struct UniformBuffer:UniformBufferData { /// will be best to call it 'Uniforms'
    UniformBuffer(Device *device = null, U *data = null) {
        this->device = device;
        this->data   = data;
        if (device && data)
            this->buffer = Buffer { device, sizeof(U), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    }
};
