#pragma once
#include <vk/vk.hpp>

struct PipelineData;
struct Render {
    Device            *device;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes
    array<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    array<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    array<VkFence>     fence_active;
    array<VkFence>     image_active;
    VkCommandBuffer    render_cmd;
    rgba               color  = {0,0,0,1};
    int                cframe = 0;
    int                sync = 0;
    
    inline void push(PipelineData &pd) {
        sequence.push(&pd);
    }
    ///
    Render(Device * = null);
    void present();
    void update();
};
