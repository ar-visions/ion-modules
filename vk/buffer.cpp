#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/buffer.hpp>

Buffer::Buffer(Device *device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo bi {};
    bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size         = VkDeviceSize(size);
    bi.usage        = usage;
    bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bi, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer!");
    ///
    /// fetch 'requirements'
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer, &req);
    ///
    /// allocate and bind
    VkMemoryAllocateInfo alloc {};
    alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize    = req.size;
    alloc.memoryTypeIndex   = findMemoryType(req.memoryTypeBits, properties);
    assert(vkAllocateMemory(device, &alloc, nullptr, &memory) == VK_SUCCESS);
    vkBindBufferMemory(device, buffer, memory, 0);
    
    info = VkDescriptorBufferInfo {};
    info.buffer = buffer;
    info.offset = 0;
    info.range  = size; /// was:sizeof(UniformBufferObject)
}

void Buffer::copy_to(Buffer &dst, size_t size) {
    auto cb = device.begin();
    VkBufferCopy params {};
    params.size = VkDeviceSize(size);
    vkCmdCopyBuffer(cb, *this, dst, 1, &params);
    device.submit(cb);
}

Buffer::operator VkDeviceMemory() {
    return memory;
}

Buffer::operator VkBuffer() {
    return buffer;
}

// has to come from a central 'Device' eh. ok.







