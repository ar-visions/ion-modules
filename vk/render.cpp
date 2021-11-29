#include <vk/vk.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>
#include <queue>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// important that only active pipelines be presented; makes perfect sense to have controls on hidden or inactive objects

static const int MAX_FRAMES_IN_FLIGHT = 2;

void Render::present() {
    auto &device = *this->device;
    vkWaitForFences(device, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
    
    uint32_t frame_index;
    VkResult result = vkAcquireNextImageKHR(device, device.swap_chain, UINT64_MAX, image_avail[cframe], VK_NULL_HANDLE, &frame_index);
    ///Frame    &frame = device.frames[frame_index];
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        assert(false); // lets see when this happens
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        assert(false); // lets see when this happens
    ///
    if (image_active[frame_index] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &image_active[frame_index], VK_TRUE, UINT64_MAX);
    
    /// iterate through render sequence
    while (sequence.size()) {
        auto &pipeline = *sequence.front();
        sequence.pop();
        
        RenderPair &pair = pipeline.pairs[frame_index];
        /// transfer uniform struct; a lambda could be called here... for now its just copying the memory
        pair.ubo.transfer();
        ///
        image_active[frame_index]        = fence_active[cframe];
        VkSubmitInfo         submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        VkSemaphore             s_wait[] = {   image_avail[cframe] };
        VkSemaphore           s_signal[] = { render_finish[cframe] };
        VkPipelineStageFlags    f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        ///
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &pair.cmd;
        submit_info.signalSemaphoreCount = 1;
        ///
        vkResetFences(device, 1, &fence_active[cframe]);
        assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
        
        VkPresentInfoKHR         present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1,
            s_signal, 1, &device.swap_chain, &frame_index };
        VkResult                 presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
        
        if (presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR || resized) {
            resized = false;
            assert(false);
            //recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
    cframe = (cframe + 1) % MAX_FRAMES_IN_FLIGHT; /// ??
}

Render::Render(Device *device): device(device) {
    if (device) {
        const int n_swap = device->swap_images.size();
        const int mx     = MAX_FRAMES_IN_FLIGHT;
        image_avail   = vec<VkSemaphore>(mx);
        render_finish = vec<VkSemaphore>(mx);
        fence_active  = vec<VkFence>(mx);
        image_active  = vec<VkFence>(n_swap);
        
          image_avail.resize(mx);
        render_finish.resize(mx);
         fence_active.resize(mx);
         image_active.resize(n_swap, VK_NULL_HANDLE); /// thar be the problem!!
        
        vec<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
        vec<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
        vec<VkFence>     fence_active;
        vec<VkFence>     image_active;
        
        VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
        
        for (size_t i = 0; i < mx; i++) {
            VkSemaphore &ia =   image_avail[i];
            VkSemaphore &rf = render_finish[i];
            VkFence     &fa =  fence_active[i];
            assert (vkCreateSemaphore(*device, &si, null, &ia) == VK_SUCCESS ||
                    vkCreateSemaphore(*device, &si, null, &rf) == VK_SUCCESS ||
                    vkCreateFence(    *device, &fi, null, &fa) == VK_SUCCESS);
        }
    }
}

