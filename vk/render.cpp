#include <vk/vk.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// important that only active pipelines be presented; makes perfect sense to have controls on hidden or inactive objects

static const int MAX_FRAMES_IN_FLIGHT = 2;

PipelineData &Render::operator[](std::string n) {
    return *pipelines[n];
}

void Render::execute() {
    /// frames and plumbing need a bit of integration
    Device &device = *this->device;
    Frame  &frame  = device.frames[cframe];
    ///
    for (auto &[n, p]: pipelines) {
        PipelineData &pipeline = *p;
        if (!pipeline.pairs)
             pipeline.pairs.resize(device.frames.size());
        RenderPair &pair = pipeline.pairs[frame.index];
        /// release and allocate command buffer (1)
        if (pair.cmd)
            vkFreeCommandBuffers(device, device.pool, 1, &pair.cmd);
        auto ai = VkCommandBufferAllocateInfo {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            null, device.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        assert(vkAllocateCommandBuffers(device, &ai, &pair.cmd) == VK_SUCCESS);
        pipeline.update(frame, pair.cmd);
    }
}

void Render::present() {
    auto &device = *this->device;
    vkWaitForFences(device, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index; /// look at the ref here
    VkResult result = vkAcquireNextImageKHR(device, device.swap_chain, UINT64_MAX, image_avail[cframe], VK_NULL_HANDLE, &image_index);
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
    
    /// iterate through render sequence
    while (sequence.size()) {
        auto &pipeline = *sequence.front();
        sequence.pop();
        
        RenderPair &pair = pipeline.pairs[image_index];
        /// transfer uniform struct; a lambda could be called here... for now its just copying the memory
        pair.ubo.transfer();
        ///
        image_active[image_index]        = fence_active[cframe];
        VkSubmitInfo         submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &pair.cmd;
        submit_info.signalSemaphoreCount = 1;

        assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
    }
    
    VkPresentInfoKHR         present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1,
        s_signal, 1, &device.swap_chain, &image_index };
    VkResult                 presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
    
    if (presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR || resized) {
        resized = false;
        assert(false);
        //recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    
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

