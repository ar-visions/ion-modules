#include <vk/vk.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        assert(false); // lets see when this happens
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        assert(false); // lets see when this happens
    ///
    if (image_active[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &image_active[image_index], VK_TRUE, UINT64_MAX);
    ///
    vkResetFences(device, 1, &fence_active[cframe]);
    
    VkSemaphore          s_wait[] = {   image_avail[cframe] };
    VkSemaphore        s_signal[] = { render_finish[cframe] };
    VkPipelineStageFlags f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    auto              pipelines   = array<PipelineData::Memory *>(sequence.size());
    Frame                &frame   = device.frames[cframe];
    const float               b   = 255.0f;
    auto            clear_color   = [&](rgba c) {
        return VkClearColorValue {{ float(c.r) / b, float(c.g) / b, float(c.b) / b, float(c.a) / b }};
    };
    ///
    /// transfer the ubo data on each and gather a VkCommandBuffer array to submit to vkQueueSubmit
    ///
    while (sequence.size()) {
        PipelineData::Memory &m = *sequence.front()->m;
        sequence.pop();
        /// send us your uniforms!
        m.ubo.transfer(image_index);
        pipelines += &m;
    }
    
    /// reallocate command
    vkFreeCommandBuffers(device, device.command, 1, &render_cmd);
    auto      alloc_info = VkCommandBufferAllocateInfo {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                null, device.command, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    assert(vkAllocateCommandBuffers(device, &alloc_info, &render_cmd) == VK_SUCCESS);
    
    /// begin command
    auto           binfo = VkCommandBufferBeginInfo {
                  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    
    assert(vkBeginCommandBuffer(render_cmd, &binfo) == VK_SUCCESS);
    
    auto         cvalues = array<VkClearValue> {
        {         .color = clear_color(color)}, // for image rgba  sfloat
        {  .depthStencil = {1.0f, 0}}};         // for image depth sfloat
    
    auto           rinfo = VkRenderPassBeginInfo { /// nothing in canvas showed with depthStencil = 0.0.
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = null,
        .renderPass      = VkRenderPass(device),
        .framebuffer     = VkFramebuffer(frame),
        .renderArea      = {{ 0, 0 }, device.extent},
        .clearValueCount = uint32_t(cvalues.size()),
        .pClearValues    = cvalues.data()
    };

    vkCmdBeginRenderPass(render_cmd, &rinfo, VK_SUBPASS_CONTENTS_INLINE);
    
    for (PipelineData::Memory *p: pipelines)
        if (p->ibo) {
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindPipeline      (render_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline);
            vkCmdBindVertexBuffers (render_cmd, 0, 1, &(VkBuffer &)(p->vbo), offsets);
            vkCmdBindIndexBuffer   (render_cmd, p->ibo, 0, p->ibo.buffer.type_size == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(render_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline_layout, 0, 1, p->desc_sets.data(), 0, nullptr);
            vkCmdDrawIndexed       (render_cmd, uint32_t(p->ibo.size()), 1, 0, 0, 0);
        }
        
    vkCmdEndRenderPass(render_cmd);
    assert(vkEndCommandBuffer(render_cmd) == VK_SUCCESS);
    
    /// submit the render command...
    image_active[image_index]        = fence_active[cframe];
    VkSubmitInfo submit_info         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = s_wait;
    submit_info.pWaitDstStageMask    = f_wait;
    submit_info.pSignalSemaphores    = s_signal;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &render_cmd;
    submit_info.signalSemaphoreCount = 1;
    assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
    ///
    VkPresentInfoKHR present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1, s_signal, 1, &device.swap_chain, &image_index };
    VkResult         presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
    
    if ((presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR)) {// && !sync_diff) {
        device.update(); /// cframe needs to be valid at its current value
    }// else
     //   assert(result == VK_SUCCESS);
    
    cframe = (cframe + 1) % MAX_FRAMES_IN_FLIGHT;
}

Render::Render(Device *device): device(device) {
    if (device) {
        const Size ns  = device->swap_images.size();
        const Size mx  = MAX_FRAMES_IN_FLIGHT;
        image_avail    = array<VkSemaphore>(mx, VK_NULL_HANDLE);
        render_finish  = array<VkSemaphore>(mx, VK_NULL_HANDLE);
        fence_active   = array<VkFence>    (mx, VK_NULL_HANDLE);
        image_active   = array<VkFence>    (ns, VK_NULL_HANDLE);
        ///
        VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
        for (size_t i = 0; i < mx; i++)
            assert (vkCreateSemaphore(*device, &si, null,   &image_avail[i]) == VK_SUCCESS &&
                    vkCreateSemaphore(*device, &si, null, &render_finish[i]) == VK_SUCCESS &&
                    vkCreateFence(    *device, &fi, null,  &fence_active[i]) == VK_SUCCESS);
    }
}

