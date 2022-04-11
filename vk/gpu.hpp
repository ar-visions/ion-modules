#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/opaque.hpp>

struct iGPU;
///
/// a good vulkan api will be fully accessible but minimally interfaced
struct GPU {
public:
    enum Capability {
        Present,
        Graphics,
        Complete
    };
    sh<iGPU> intern; /// hide dependencies with shared pointers.  it makes them worth looking at, and more.
    ///
    GPU(VkPhysicalDevice &, VkSurfaceKHR &);
    GPU(std::nullptr_t n = null) { }
    GPU &operator=(const GPU &gpu);
    uint32_t                index(Capability);
    void                    destroy();
    operator                bool();
    operator                VkPhysicalDevice &();
    bool                    has_capability(Capability);
    static array<GPU>       listing();
    static GPU              select(int index = 0);
};
