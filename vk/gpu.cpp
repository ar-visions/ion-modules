#include <dx/dx.hpp>
#include <vk/vk.hpp>

VkSampleCountFlagBits max_sampling(VkPhysicalDevice gpu) {
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

/// surface handle passed in for all gpus, its going to be the same one for each
GPU::GPU(VkPhysicalDevice gpu, VkSurfaceKHR surface) : gpu(gpu) {
    uint32_t fcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, nullptr);
    family_props.resize(fcount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, family_props.data());
    uint32_t index = 0;
    /// we need to know the indices of these
    gfx     = var(false);
    present = var(false);
    
    for (const auto &fprop: family_props) { // check to see if it steps through data here.. thats the question
        VkBool32 has_present;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &has_present);
        bool has_gfx = (fprop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        ///
        /// prefer to get gfx and present in the same family_props
        if (has_gfx) {
            gfx = var(index);
            /// query surface formats
            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
            if (format_count != 0) {
                support.formats.resize(format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                    gpu, surface, &format_count, support.formats.data());
            }
            /// query swap chain support, store on GPU
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                gpu, surface, &support.caps);
            /// ansio relates to gfx
            VkPhysicalDeviceFeatures fx;
            vkGetPhysicalDeviceFeatures(gpu, &fx);
            support.ansio = fx.samplerAnisotropy;
        }
        ///
        if (has_present) {
            present = var(index);
            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                gpu, surface, &present_mode_count, nullptr);
            if (present_mode_count != 0) {
                support.present_modes.resize(present_mode_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                    gpu, surface, &present_mode_count, support.present_modes.data());
            }
        }
        if (has_gfx & has_present)
            break;
        index++;
    }
    if (support.ansio)
        support.max_sampling = max_sampling(gpu);
}

vec<GPU> GPU::listing() {
    VkInstance vk = Vulkan::instance();
    uint32_t   gpu_count;
    vkEnumeratePhysicalDevices(vk, &gpu_count, null);
    auto      gpu = vec<GPU>();
    auto       hw = vec<VkPhysicalDevice>();
    gpu.resize(gpu_count);
    hw.resize(gpu_count);
    vkEnumeratePhysicalDevices(vk, &gpu_count, hw.data());
    vec2i sz = Vulkan::startup_rect().size();
    VkSurfaceKHR surface = Vulkan::surface(sz);
    for (size_t i = 0; i < gpu_count; i++) {
        gpu[i] = GPU(hw[i], surface);
    }
    return gpu;
}

GPU GPU::select(int index) {
    vec<GPU> gpu = GPU::listing();
    assert(index >= 0 && index < gpu.size());
    return gpu[index];
}

GPU::operator VkPhysicalDevice() {
    return gpu;
}

bool GPU::operator()(Capability caps) {
    bool g = gfx     == Type::ui32;
    bool p = present == Type::ui32;
    if (caps == Complete)
        return g && p;
    if (caps == Graphics)
        return g;
    if (caps == Present)
        return p;
    return true;
}

uint32_t GPU::index(Capability caps) {
    assert(gfx == Type::ui32 || present == Type::ui32);
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

