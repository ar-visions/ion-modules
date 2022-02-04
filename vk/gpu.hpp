#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>

///
/// a good vulkan api will be fully accessible but minimally interfaced
struct GPU {
protected:
    var gfx;
    var present;
public:
    enum Capability {
        Present,
        Graphics,
        Complete
    };
    struct Support {
        VkSurfaceCapabilitiesKHR caps;
        array<VkSurfaceFormatKHR>  formats;
        array<VkPresentModeKHR>    present_modes;
        bool                     ansio;
        VkSampleCountFlagBits    max_sampling;
    };
    Support          support;
    VkPhysicalDevice gpu        = VK_NULL_HANDLE;
    VkQueue          queues[2];
    array<VkQueueFamilyProperties> family_props;
    VkSurfaceKHR     surface;
    ///
    GPU(VkPhysicalDevice, VkSurfaceKHR);
    GPU(std::nullptr_t n = null) { }
    uint32_t                index(Capability);
    void                    destroy();
    operator                bool();
    operator                VkPhysicalDevice();
    bool                    operator()(Capability);
    static array<GPU>         listing();
    static GPU              select(int index = 0);
};
