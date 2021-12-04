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
        type_size = sizeof(T);
        VkDevice device = device_handle(d);
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


