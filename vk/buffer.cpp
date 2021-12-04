#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/buffer.hpp>

Buffer::Buffer(Device *device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
        device(device), sz(size) {
    VkBufferCreateInfo bi {};
    bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size         = VkDeviceSize(size);
    bi.usage        = usage;
    bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;
    ///
    assert(vkCreateBuffer(*device, &bi, nullptr, &buffer) == VK_SUCCESS);
    ///
    /// fetch 'requirements'
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(*device, buffer, &req);
    ///
    /// allocate and bind
    VkMemoryAllocateInfo alloc {};
    alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize    = req.size;
    alloc.memoryTypeIndex   = device->memory_type(req.memoryTypeBits, properties);
    assert(vkAllocateMemory(*device, &alloc, nullptr, &memory) == VK_SUCCESS);
    vkBindBufferMemory(*device, buffer, memory, 0);
    ///
    info = VkDescriptorBufferInfo {};
    info.buffer = buffer;
    info.offset = 0;
    info.range  = size; /// was:sizeof(UniformBufferObject)
}

/// no crop support yet; simple
void Buffer::copy_to(Texture *tx) {
    auto device = *this->device;
    auto cmd    = device.begin();
    auto reg    = VkBufferImageCopy {};
    reg.imageSubresource = { tx->aflags, 0, 0, 1 }; /// you moron.
    reg.imageExtent      = { uint32_t(tx->sz.x), uint32_t(tx->sz.y), 1 };
    vkCmdCopyBufferToImage(cmd, buffer, *tx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
    device.submit(cmd);
}

void Buffer::copy_to(Buffer &dst, size_t size) {
    auto cb = device->begin();
    VkBufferCopy params {};
    params.size = VkDeviceSize(size);
    vkCmdCopyBuffer(cb, *this, dst, 1, &params);
    device->submit(cb);
}

Buffer::operator VkDeviceMemory() {
    return memory;
}

Buffer::operator VkBuffer() {
    return buffer;
}

void Buffer::destroy() {
    vkDestroyBuffer(*device, buffer, null);
    vkFreeMemory(*device, memory, null);
}

// has to come from a central 'Device' eh. ok.







