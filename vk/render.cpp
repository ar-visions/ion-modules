#include <vk/vk.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// important that only active pipelines be presented; makes perfect sense to have controls on hidden or inactive objects

static const int MAX_FRAMES_IN_FLIGHT = 2;

/// the thing that 'executes' is the present.  so this is just 'update'
void Render::update() {
    Device &device = *this->device;
    Frame  &frame  = device.frames[cframe];
    assert (cframe == frame.index);
}

void Render::present() {
    Device &device = *this->device;
    vkWaitForFences(device, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index; /// look at the ref here
    VkResult result = vkAcquireNextImageKHR(
        device, device.swap_chain, UINT64_MAX,
        image_avail[cframe], VK_NULL_HANDLE, &image_index);
    ///Frame    &frame = device.frames[image_index];
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        assert(false); // lets see when this happens
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        assert(false); // lets see when this happens
    ///
    if (image_active[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &image_active[image_index], VK_TRUE, UINT64_MAX);
    ///
    vkResetFences(device, 1, &fence_active[cframe]);
    VkSemaphore             s_wait[] = {   image_avail[cframe] };
    VkSemaphore           s_signal[] = { render_finish[cframe] };
    VkPipelineStageFlags    f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    bool sync_diff = false;
    ///
    /// iterate through render sequence, updating out of sync pipelines
    while (sequence.size()) {
        auto &m = *sequence.front()->m;
        sequence.pop();
        
        if (m.sync != sync) {
            /// Device::update() calls change the sync (so initial and updates)
            sync_diff = true;
            m.sync    = sync;
            if (!m.frame_commands)
                 m.frame_commands = vec<VkCommandBuffer> (
                    device.frames.size(), VK_NULL_HANDLE
                 );
            for (size_t i = 0; i < device.frames.size(); i++) //
                m.update(i); /// make singular, frame index param
        }
        
        /// transfer uniform struct from lambda call. send us your uniforms!
        m.ubo.transfer(image_index);
        
        ///
        image_active[image_index]        = fence_active[cframe];
        VkSubmitInfo submit_info         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &m.frame_commands[image_index];
        submit_info.signalSemaphoreCount = 1;
        assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
        int test = 0;
        test++;
    }
    ///
    VkPresentInfoKHR     present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1,
        s_signal, 1, &device.swap_chain, &image_index };
    VkResult             presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
    ///
    if ((presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR) && !sync_diff) {
        device.update(); /// cframe needs to be valid at its current value
    } else
        assert(result == VK_SUCCESS);
    
    cframe = (cframe + 1) % MAX_FRAMES_IN_FLIGHT; /// ??
}

Render::Render(Device *device): device(device) {
    if (device) {
        const int ns  = device->swap_images.size();
        const int mx  = MAX_FRAMES_IN_FLIGHT;
        image_avail   = vec<VkSemaphore>(mx, VK_NULL_HANDLE);
        render_finish = vec<VkSemaphore>(mx, VK_NULL_HANDLE);
        fence_active  = vec<VkFence>    (mx, VK_NULL_HANDLE);
        image_active  = vec<VkFence>    (ns, VK_NULL_HANDLE);
        
        VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
        
        for (size_t i = 0; i < mx; i++) {
            VkSemaphore &ia =   image_avail[i];
            VkSemaphore &rf = render_finish[i];
            VkFence     &fa =  fence_active[i];
            assert (vkCreateSemaphore(*device, &si, null, &ia) == VK_SUCCESS &&
                    vkCreateSemaphore(*device, &si, null, &rf) == VK_SUCCESS &&
                    vkCreateFence(    *device, &fi, null, &fa) == VK_SUCCESS);
        }
    }
}

