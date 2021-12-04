#pragma once
#include <vk/vk.hpp>

struct PipelineData;
struct Render {
    Device          *device;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes perhaps
    vec<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    vec<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    vec<VkFence>     fence_active;
    vec<VkFence>     image_active;
    int              cframe = 0;
    bool             resized;
    ///
    std::map<std::string, PipelineData *> pipelines;
    PipelineData &operator[](std::string n);
    ///
    Render(Device * = null);
    void present();
    void execute();
};
