#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/device.hpp>

struct Device;
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
    
    Buffer(Device *, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    void copy_to(Buffer &, size_t);
};
