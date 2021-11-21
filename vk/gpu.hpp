#pragma once
#include <data/data.hpp>
#include <vk/vk.hpp>

///
/// a good vulkan api will be fully accessible but minimally interfaced
struct GPU {
protected:
    var              gfx;
    var              present;
public:
    enum Capability {
        Present,
        Graphics,
        Complete
    };
    VkPhysicalDevice gpu;
    VkQueue          queues[2];
    
    ///
    GPU(VkPhysicalDevice);
    uint32_t                index(Capability);
    void                    destroy();
    operator                VkPhysicalDevice();
    bool                    operator()(Capability);
    VkSampleCountFlagBits   max_sampling();
};

