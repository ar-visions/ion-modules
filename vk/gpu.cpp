#include <dx/dx.hpp>
#include <vk/vk.hpp>

/// surface handle passed in for all gpus, its going to be the same one for each
GPU::GPU(VkPhysicalDevice gpu, VkSurfaceKHR surface) : surface(surface), gpu(gpu) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, families.data());
    uint32_t index = 0;
    for (const auto &family: families) {
        VkBool32 has_present;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &has_present);
        gfx     = !!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? var(index) : var(false);
        present = has_present ? var(index) : var(false);
        bool  g = gfx.t   == var::ui32;
        bool  p = present == var::ui32;
        
        /// query swap chain support, store on GPU
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            gpu, surface, &support.caps);
        ///
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            gpu, surface, &format_count, nullptr);
        ///
        if (format_count != 0) {
            support.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                gpu, surface, &format_count, support.formats);
        }
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            gpu, surface, &present_mode_count, nullptr);
        ///
        if (present_mode_count != 0) {
            support.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                gpu, surface, &present_mode_count, support.present_modes);
        }
        VkPhysicalDeviceFeatures fx;
        vkGetPhysicalDeviceFeatures(gpu, &fx);
        support.ansio = fx.samplerAnisotropy;
        ///
        if (g & p)
            break;
        index++;
    }
}

bool GPU::operator()(Capability caps) {
    bool g = gfx.t   == var::ui32;
    bool p = present == var::ui32;
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
    if (caps == Present  && (*this)(Present))
        return uint32_t(present);
    assert(false);
    return 0;
}


GPU::operator bool() {
    return (*this)(Complete) && support.formats && support.present_modes && support.ansio;
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
