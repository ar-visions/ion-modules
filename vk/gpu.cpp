#pragma once
#include <data/data.hpp>
#include <vk/vk.hpp>

GPU::GPU(VkPhysicalDevice gpu) : gpu(gpu) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families.data());
    int index = 0;
    for (const auto &family: families) {
        bool has_present;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &has_present);
        gfx     = !!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? var(index) : var(false);
        present = has_present ? var(index) : var(false);
        bool  g = gfx.t   == Data::ui32;
        bool  p = present == Data::ui32;
        if (g & p)
            break;
        index++;
    }
    return indices;
}

bool GPU::operator()(Capability caps) {
    bool g = gfx.t   == Data::ui32;
    bool p = present == Data::ui32;
    if (caps == Complete)
        return g && p;
    if (caps == Graphics)
        return g;
    if (caps == Present)
        return p;
    return true;
}

uint32_t GPU::index(Capability caps) {
    assert(gfx || present);
    if (caps == Graphics && (*this)(Graphics))
        return uint32_t(gfx);
    if (caps == Present  && (*this)(Presentation))
        return uint32_t(present);
    assert(false);
    return 0;
}

GPU::operator bool() {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool ext = checkDeviceExtensionSupport(device);
    bool swap = false;
    if (ext) {
        SwapChainSupportDetails support = querySwapChainSupport(device);
        swap = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return (*this)(Complete) && ext && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
}

VkSampleCountFlagBits GPU::max_sampling() {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(gpu, &props);
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                props.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }
    return VK_SAMPLE_COUNT_1_BIT;
}
