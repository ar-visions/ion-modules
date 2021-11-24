#include <vk/vk.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

void Frame::create(vec<VkImageView> &attachments) {
    VkFramebufferCreateInfo ci {};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = device->render_pass;
    ci.attachmentCount = static_cast<uint32_t>(attachments.size());
    ci.pAttachments    = attachments.data();
    ci.width           = device->extent.width;
    ci.height          = device->extent.height;
    ci.layers          = 1;
    assert (vkCreateFramebuffer(*device, &ci, nullptr, &framebuffer) == VK_SUCCESS);
}

uint32_t Device::memory_type(uint32_t types, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mprops;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mprops);
    ///
    for (uint32_t i = 0; i < mprops.memoryTypeCount; i++)
        if ((types & (1 << i)) && (mprops.memoryTypes[i].propertyFlags & props) == props)
            return i;
    /// no flags match
    assert(false);
    return 0;
};

Device::Device(GPU *p_gpu, bool aa, bool val) : gpu(*p_gpu) {
    vec<VkDeviceQueueCreateInfo> vq;
    vec<uint32_t> families = {
        gpu.index(GPU::Graphics),
        gpu.index(GPU::Present)
    };
    
    float priority = 1.0f;
    for (uint32_t family: families) {
        VkDeviceQueueCreateInfo q {};
        q.sType                = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex     = family;
        q.queueCount           = 1;
        q.pQueuePriorities     = &priority;
        vq                    += q;
    }

    VkPhysicalDeviceFeatures feat {};
    feat.samplerAnisotropy     = aa ? VK_TRUE : VK_FALSE;
    ///
    uint32_t  esz;
    auto glfw_ext = (const char **)glfwGetRequiredInstanceExtensions(&esz);
    auto      ext = vec<const char *>(esz);
    for (int    i = 0; i < esz; i++)
        ext      += glfw_ext[i];
    ///
    if (val)
        ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    ///
    VkDeviceCreateInfo ci {};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = uint32_t(vq.size());
    ci.pQueueCreateInfos       = vq;
    ci.pEnabledFeatures        = &feat;
    ci.enabledExtensionCount   = uint32_t(ext.size());
    ci.ppEnabledExtensionNames = ext;

#ifndef NDEBUG
    if (val) {
        static const char *debug   = "VK_LAYER_KHRONOS_validation";
        ci.enabledLayerCount       = 1;
        ci.ppEnabledLayerNames     = &debug;
    }
#endif

    assert(vkCreateDevice(gpu, &ci, nullptr, &device) == VK_SUCCESS);
    vkGetDeviceQueue(device, gpu.index(GPU::Graphics), 0, &queues[GPU::Graphics]);
    vkGetDeviceQueue(device, gpu.index(GPU::Present),  0, &queues[GPU::Present]);
}

VkImageView Device::create_iview(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageView view;
    auto             v = VkImageViewCreateInfo {};
    v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    v.image            = image;
    v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    v.format           = format;
    v.subresourceRange = { aspectFlags, 0, mipLevels, 0, 1 };
    assert (vkCreateImageView(device, &v, nullptr, &view) == VK_SUCCESS);
    return view;
}

/// setup everything in the Framebuffer vector
void Device::create_swapchain()
{
    auto select_surface_format = [](vec<VkSurfaceFormatKHR> &formats) -> VkSurfaceFormatKHR {
        for (const auto &f: formats)
            if (f.format     == VK_FORMAT_B8G8R8A8_SRGB &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        return formats[0];
    };
    ///
    auto select_present_mode = [](vec<VkPresentModeKHR> &modes) -> VkPresentModeKHR {
        for (const auto &m: modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    };
    ///
    auto swap_extent = [](VkSurfaceCapabilitiesKHR& caps) -> VkExtent2D {
        if (caps.currentExtent.width != UINT32_MAX) {
            return caps.currentExtent;
        } else {
            int w, h;
            glfwGetFramebufferSize(*Window::first, &w, &h);
            VkExtent2D ext = { uint32_t(w), uint32_t(h) };
            ext.width      = std::clamp(ext.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
            ext.height     = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            return ext;
        }
    };
    ///
    auto     support              = gpu.support;
    uint32_t gfx_index            = gpu.index(GPU::Graphics);
    uint32_t present_index        = gpu.index(GPU::Present);
    auto     surface_format       = select_surface_format(support.formats);
    auto     surface_present      = select_present_mode(support.present_modes);
    auto     extent               = swap_extent(support.caps);
    uint32_t frame_count          = support.caps.minImageCount + 1;
    uint32_t indices[]            = { gfx_index, present_index };
    ///
    if (support.caps.maxImageCount > 0 && frame_count > support.caps.maxImageCount)
        frame_count               = support.caps.maxImageCount;
    ///
    VkSwapchainCreateInfoKHR ci {};
    ci.sType                      = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface                    = gpu.surface;
    ci.minImageCount              = frame_count;
    ci.imageFormat                = surface_format.format;
    ci.imageColorSpace            = surface_format.colorSpace;
    ci.imageExtent                = extent;
    ci.imageArrayLayers           = 1;
    ci.imageUsage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ///
    if (gfx_index != present_index) {
        ci.imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount  = 2;
        ci.pQueueFamilyIndices    = indices;
    } else
        ci.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    ///
    ci.preTransform               = support.caps.currentTransform;
    ci.compositeAlpha             = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode                = surface_present;
    ci.clipped                    = VK_TRUE;
    ///
    /// create swap-chain
    assert(vkCreateSwapchainKHR(device, &ci, nullptr, &swap_chain) == VK_SUCCESS);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, nullptr);
    frames.resize(frame_count);
    ///
    /// get swap-chain images
    vec<VkImage> swap_images;
    swap_images.resize(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format                        = surface_format.format;
    this->extent                  = extent;
    viewport                      = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    ///
    /// create descriptor pool
    std::array<VkDescriptorPoolSize, 2> ps {};
    ps[0]                         = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(frame_count) };
    ps[1]                         = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(frame_count) };
    VkDescriptorPoolCreateInfo pi = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                                      uint32_t(frame_count), uint32_t(ps.size()), ps.data() };
    pi.poolSizeCount              = uint32_t(ps.size());
    pi.pPoolSizes                 = ps.data();
    pi.maxSets                    = uint32_t(frame_count);
    assert(vkCreateDescriptorPool(device, &pi, nullptr, &desc.pool) == VK_SUCCESS);
    ///
    /// create view and uniform buffers
    for (size_t i = 0; i < frame_count; i++) {
        Frame &f  = frames[i];
        VkImageView view = create_iview(swap_images[i], format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        f.tx      = Texture { this, { int(extent.width), int(extent.height) }, swap_images[i], view };
        f.uniform = UniformBuffer<Uniform>(this);
        ///
        vec<VkImageView> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        ///
        auto ci = f(attachments);
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create framebuffer!");
    }
}

void Device::create_color_resources() {
    
    tx_color = Texture( vec2i { extent.width, extent.height }, format,  )
    
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void Device::create_depth_resources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}


void Device::create_render_pass() {
    VkAttachmentDescription color {};
    color.format                = format;
    color.samples               = msaa_samples;
    color.loadOp                = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp               = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth {};
    depth.format                = find_depth_format();
    depth.samples               = msaa_samples;
    depth.loadOp                = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription cres {};
    cres.format                 = format;
    cres.samples                = VK_SAMPLE_COUNT_1_BIT;
    cres.loadOp                 = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    cres.storeOp                = VK_ATTACHMENT_STORE_OP_STORE;
    cres.stencilLoadOp          = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    cres.stencilStoreOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    cres.initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
    cres.finalLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference cref {};
    cref.attachment             = 0;
    cref.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference dref {};
    dref.attachment             = 1;
    dref.layout                 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference rref {};
    rref.attachment             = 2;
    rref.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sp {};
    sp.pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount     = 1;
    sp.pColorAttachments        = &cref;
    sp.pDepthStencilAttachment  = &dref;
    sp.pResolveAttachments      = &rref;

    VkSubpassDependency dep {};
    dep.srcSubpass              = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass              = 0;
    dep.srcStageMask            = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask           = 0;
    dep.dstStageMask            = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask           = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {color, depth, cres};
    VkRenderPassCreateInfo rpi {};
    rpi.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount         = uint32_t(attachments.size());
    rpi.pAttachments            = attachments.data();
    rpi.subpassCount            = 1;
    rpi.pSubpasses              = &sp;
    rpi.dependencyCount         = 1;
    rpi.pDependencies           = &dep;

    assert(vkCreateRenderPass(device, &rpi, nullptr, &render_pass) == VK_SUCCESS);
}


void Device::create_command_buffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }
}

VkCommandBuffer Device::begin() {
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo ai {};
    ///
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool        = pool;
    ai.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &ai, &cb);
    ///
    VkCommandBufferBeginInfo bi {};
    bi.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    return cb;
}

void Device::submit(VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);
    ///
    VkSubmitInfo si {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cb;
    ///
    VkQueue &queue          = queues[GPU::Graphics];
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    ///
    vkFreeCommandBuffers(device, pool, 1, &cb);
}

/// remember to call initialize after you select a device
/// note that this requires explicit destroy afterwards
void Device::initialize() {
    VkCommandPoolCreateInfo pi {};
    pi.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pi.queueFamilyIndex = gpu.indices(GPU::Graphics);
    assert(vkCreateCommandPool(device, &pi, nullptr, &pool) == VK_SUCCESS);
    

}

VkQueue &Device::operator()(GPU::Capability cap) {
    assert(cap != GPU::Complete);
    return queues[cap];
}

Device::operator VkPhysicalDevice() {
    return gpu;
}

Device::operator VkCommandPool() {
    return pool;
}

Device::operator VkDevice() {
    return device;
}

Device::operator VkDescriptorPool() {
    return desc_pool;
}

///
void Device::destroy() {
    ///
    vkDestroyImageView (device, depthImageView,   nullptr);
    vkDestroyImage     (device, depthImage,       nullptr);
    vkFreeMemory       (device, depthImageMemory, nullptr);
    vkDestroyImageView (device, colorImageView,   nullptr);
    vkDestroyImage     (device, colorImage,       nullptr);
    vkFreeMemory       (device, colorImageMemory, nullptr);
    ///
    for (auto framebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device,       graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device,     renderPass, nullptr);

    for (auto imageView : swapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, desc_pool, nullptr);
}

///
void Frame::destroy(VkDevice &device) {
    for (auto &fb: frames) {
        vkDestroyFramebuffer(device, fb.framebuffer, nullptr);
        vkDestroyImageView  (device, fb.view,        nullptr);
        vkDestroyImage      (device, fb.image,       nullptr); /// this may be free'd twice so watch it.
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device,       graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device,     renderPass, nullptr);

    for (auto imageView : swapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    ///vkDestroyDescriptorPool(device, desc_pool, nullptr);
}
