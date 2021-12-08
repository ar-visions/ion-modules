#pragma once
#include <vk/vk.hpp>

struct PipelineData;
struct Render {
    Device          *device;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes
    vec<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    vec<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    vec<VkFence>     fence_active;
    vec<VkFence>     image_active;
    int              cframe = 0;
    int              sync = 0;
    
    inline void push(PipelineData &pd) {
        sequence.push(&pd);
    }
    ///
    Render(Device * = null);
    void present();
    void update();
};
