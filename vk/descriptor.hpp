#pragma once
#include <vk/vk.hpp>

/// worth having; rid ourselves of more terms =)
struct Descriptor {
    Device               *device;
    VkDescriptorPool      pool;
    void     destroy();
    Descriptor(Device *device = null) : device(device) { }
};
