#pragma once
#include <data/data.hpp>
#include <vk/vk.hpp>

///
/// a good vulkan api will be fully accessible but minimally interfaced
struct Buffer {
    VkBuffer       buffer;
    VkDeviceMemory memory;
    size_t         sz;
    ///
    void           destroy();
    operator       VkBuffer();
    operator       VkDeviceMemory();
    
    void Buffer::copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    Buffer(Device &, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    void copy_to(Buffer &dst, size_t size);
};
