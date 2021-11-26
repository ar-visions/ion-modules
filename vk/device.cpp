#include <vk/vk.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

Pipeline &Plumbing::operator[](std::string n) {
    return *pipelines[n];
}

Pipeline &Device::operator[](std::string n) {
    return plumbing[n];
}

void Plumbing::update(Frame &frame) {
    Device &device = *this->device;
    for (auto &[n, p]: pipelines) {
        if (frame.render_commands.count(n))
            vkFreeCommandBuffers(device, pool, 1, &frame.render_commands[n]);
        auto ai = VkCommandBufferAllocateInfo {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            null, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        assert(vkAllocateCommandBuffers(device, &ai, &frame.render_commands[n]) == VK_SUCCESS);
        Pipeline  &pipeline = plumbing[n];
        VkCommandBuffer &rc = frame.render_commands[n];
        pipeline.update(rc);
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

/// essentially a series of tubes...
void Device::update() {
    Pipeline &pipeline = *p;
    uint32_t  frame_count = frames.size();
    ///
    /// create VkCommandBuffer for pipelines across all frames
    /// create view, uniform buffer, render command
    for (size_t i = 0; i < frame_count; i++) {
        VkImage     image = swap_images[i];
        VkImageView  view = Texture::create_view(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        Frame      &frame = frames[i];
        if (frame.tx)
            frame.tx.destroy();
        frame.tx          = Texture { this, { int(extent.width), int(extent.height) }, image, view };
        if (frame.uniform)
            frame.uniform.destroy();
        frame.uniform     = UniformBuffer<Uniform>(this);
        frame.create({ tx_color, tx_depth, frame.tx });
        plumbing.update(frame);
    }
}

/// a series of tubes...
void Device::plumb() {
    Pipeline &pipeline    = *p;
    uint32_t  frame_count = frames.size();
    ///
    /// create VkCommandBuffer for pipelines across all frames
    /// create view, uniform buffer, render command
    for (size_t i = 0; i < frame_count; i++) {
        VkImage       image = swap_images[i];
        VkImageView    view = Texture::create_view(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        Frame        &frame = frames[i];
        frame.tx            = Texture { this, { int(extent.width), int(extent.height) }, image, view };
        frame.uniform       = UniformBuffer<Uniform>(this);
        frame.create({ tx_color, tx_depth, frame.tx });
        frame.pipeline      = &pipeline; /// will need to be a vector or map
        ///
        VkCommandBuffer &rc = frame.render_command;
        vkFreeCommandBuffers(device, pool, 1, &rc);
        /// needs to delete previous tubing
        auto     alloc_info = VkCommandBufferAllocateInfo {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, null, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        assert(vkAllocateCommandBuffers(device, &alloc_info, &rc) == VK_SUCCESS);
        ///
        auto     begin_info = VkCommandBufferBeginInfo {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        assert(vkBeginCommandBuffer(rc, &begin_info) == VK_SUCCESS);
        /// consecutive ones intra frame would have an alpha as 0 but its a bit simpleton right now
        /// this needs to be called when a pipeline is initializing
        pipeline(frame, rc);
        assert (vkEndCommandBuffer(rc) == VK_SUCCESS);
    }
}

void Device::initialize_swap(Pipeline &pipeline) {
    plumbing = Plumbing { this };
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
    ///
    /// get swap-chain images
    vec<VkImage> swap_images;
    swap_images.resize(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format                        = surface_format.format;
    this->extent                  = extent;
    viewport                      = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    
    uint32_t frame_count = frames.size();
    ///
    /// create descriptor pool
    std::array<VkDescriptorPoolSize, 2> ps {};
    ps[0]                         = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame_count };
    ps[1]                         = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count };
    VkDescriptorPoolCreateInfo pi = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                                      frame_count, uint32_t(ps.size()), ps.data() };
    pi.poolSizeCount              = uint32_t(ps.size());
    pi.pPoolSizes                 = ps.data();
    pi.maxSets                    = frame_count;
    assert(vkCreateDescriptorPool(device, &pi, nullptr, &desc.pool) == VK_SUCCESS);
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
