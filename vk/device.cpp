#include <vk/vk.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


VkWriteDescriptorSet UniformBufferData::operator()(VkDescriptorSet &ds) {
    return {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, null, ds, 0, 0,
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, null, &buffer.info, null
    };
}

void UniformBufferData::destroy() {
    buffer.destroy();
}

/// called by Pipeline, called by the Render
void UniformBufferData::transfer() {
    void* gpu;
      vkMapMemory(*device, buffer, 0, buffer.sz, 0, &gpu);
           memcpy( gpu,    data,      buffer.sz);
    vkUnmapMemory(*device, buffer);
}

void PipelineData::update(Frame &frame, VkCommandBuffer &rc) {
    Device &device = *this->device;
    auto ai = VkCommandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        null, device.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    assert(vkAllocateCommandBuffers(device, &ai, &rc) == VK_SUCCESS);
    ///
    auto   clear_values = vec<VkClearValue> {
        {        .color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        { .depthStencil = {1.0f, 0}}};
    auto    render_info = VkRenderPassBeginInfo {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, null, device, frame,
        {{0,0}, device.extent}, uint32_t(clear_values.size()), clear_values };
    ///
    vkCmdBeginRenderPass(rc, &render_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(rc, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vec<VkBuffer>    a_vbo   = { vbo };
    VkDeviceSize   offsets[] = {0};
    vkCmdBindVertexBuffers(rc, 0, 1, a_vbo.data(), offsets);
    vkCmdBindIndexBuffer(rc, ibo, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(rc, VK_PIPELINE_BIND_POINT_GRAPHICS,  layout, 0, 1, desc_sets, 0, nullptr);
    vkCmdDrawIndexed(rc, uint32_t(ibo.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(rc);
}

VkDevice device_handle(Device *device) {
    return device->device;
}

VkPhysicalDevice gpu_handle(Device *device) {
    return device->gpu;
}

uint32_t memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props) {
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

PipelineData &Plumbing::operator[](std::string n) {
    return *pipelines[n];
}

PipelineData &Device::operator[](std::string n) {
    return plumbing[n];
}

/// frames and plumbing need a bit of integration
void Plumbing::update(Frame &frame) {
    Device &device = *this->device;
    ///
    for (auto &[n, p]: pipelines) {
        PipelineData &pipeline = *p;
        if (!pipeline.pairs)
             pipeline.pairs.resize(device.frames.size());
        RenderPair &pair = pipeline.pairs[frame.index];
        ///
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

void Frame::create(vec<VkImageView> attachments) {
    auto &device = *this->device;
    VkFramebufferCreateInfo ci {};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = device.render_pass;
    ci.attachmentCount = static_cast<uint32_t>(attachments.size());
    ci.pAttachments    = attachments.data();
    ci.width           = device.extent.width;
    ci.height          = device.extent.height;
    ci.layers          = 1;
    assert (vkCreateFramebuffer(device, &ci, nullptr, &framebuffer) == VK_SUCCESS);
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

Device::Device(GPU &p_gpu, bool aa) : render(this), desc(this), gpu(p_gpu), plumbing(this) {
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
    
    vec<const char *> ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkDeviceCreateInfo ci {};
#ifndef NDEBUG
    ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    static const char *debug   = "VK_LAYER_KHRONOS_validation";
    ci.enabledLayerCount       = 1;
    ci.ppEnabledLayerNames     = &debug;
#endif
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = uint32_t(vq.size());
    ci.pQueueCreateInfos       = vq;
    ci.pEnabledFeatures        = &feat;
    ci.enabledExtensionCount   = uint32_t(ext.size());
    ci.ppEnabledExtensionNames = ext;

    assert(vkCreateDevice(gpu, &ci, nullptr, &device) == VK_SUCCESS);
    vkGetDeviceQueue(device, gpu.index(GPU::Graphics), 0, &queues[GPU::Graphics]);
    vkGetDeviceQueue(device, gpu.index(GPU::Present),  0, &queues[GPU::Present]);
}

/// essentially a series of tubes...
void Device::update() {
    uint32_t  frame_count = frames.size();
    ///
    /// create VkCommandBuffer for pipelines across all frames
    /// create view, uniform buffer, render command
    for (size_t i = 0; i < frame_count; i++) {
        VkImage     image = swap_images[i];
        VkImageView  view = Texture::create_view(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        Frame      &frame = frames[i];
        if (frame.index >= 0)
            frame.tx.destroy();
        frame.index       = i;
        frame.tx          = Texture { this, { int(extent.width), int(extent.height) }, image, view };
        if (!frame.ubo)
             frame.ubo    = UniformBuffer<Uniform>(this, &frame.uniform); /// this uniform is part of the frame display
        frame.create({ tx_color, tx_depth, frame.tx });
        
        plumbing.update(frame);
    }
}

void Device::initialize() {
    /// create command pool (i think the pooling should be on each Frame)
    VkCommandPoolCreateInfo pi {};
    pi.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pi.queueFamilyIndex = gpu.index(GPU::Graphics);
    assert(vkCreateCommandPool(device, &pi, nullptr, &pool) == VK_SUCCESS);
    
    vec2i sz = vec2i { int(extent.width), int(extent.height) };
    /// we really need implicit flagging on these if we can manage it.
    tx_color = Texture { this, sz, null,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, /// other: VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        msaa_samples,
        format, -1 };
    ///
    tx_depth = Texture { this, sz, null,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        msaa_samples,
        format, -1 };
    
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
    /// -- from here we know the majestic 'swap buffer count' and contain that in an abstract vec<Frame>
    /// -- all pipeline is commanded to these Frame anchor points of sort
    /// get swap-chain images
    swap_images.resize(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format                        = surface_format.format;
    this->extent                  = extent;
    viewport                      = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    
    ///
    /// create descriptor pool
    std::array<VkDescriptorPoolSize, 2> ps {};
    ps[0]              = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame_count };
    ps[1]              = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count };
    auto dpi           = VkDescriptorPoolCreateInfo {
                            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                            frame_count, uint32_t(ps.size()), ps.data() };
    dpi.poolSizeCount  = uint32_t(ps.size());
    dpi.pPoolSizes     = ps.data();
    dpi.maxSets        = frame_count;
    
    assert(vkCreateDescriptorPool(device, &dpi, nullptr, &desc.pool) == VK_SUCCESS);
    for (auto &f: frames)
        f.index = -1;
    
    update();
    
}

void Descriptor::destroy() {
    auto &device = *this->device;
    vkDestroyDescriptorPool(device, pool, nullptr);
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
}

VkFormat supported_format(VkPhysicalDevice gpu, vec<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &props);

        bool fmatch = (props.linearTilingFeatures & features) == features;
        bool tmatch = (tiling == VK_IMAGE_TILING_LINEAR || tiling == VK_IMAGE_TILING_OPTIMAL);
        if  (fmatch && tmatch)
            return format;
    }

    throw std::runtime_error("failed to find supported format!");
}

void Device::create_render_pass() {
    auto depth_format = [](VkPhysicalDevice gpu) -> VkFormat {
        return supported_format(gpu,
            {VK_FORMAT_D32_SFLOAT,    VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
             VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    };
    
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
    depth.format                = depth_format(gpu); /// simplify or store
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

Device::operator VkRenderPass() {
    return render_pass;
}

///
void Device::destroy() {
    for (auto &f: frames)
        f.destroy();
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    desc.destroy();
}

///
void Frame::destroy() {
    auto &device = *this->device;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    tx.destroy();
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

