#include <vk/vk.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


static Device d_null;
Device &Device::null_device() {
    return d_null;
}

VkWriteDescriptorSet UniformData::write_desc(size_t frame_index, VkDescriptorSet &ds) {
    auto &m = *this->m;
    return {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, null, ds, 0, 0,
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, null, &m.buffers[frame_index].info, null
    };
}

/// lazy initialization of Uniforms make the component tree work a bit better
void UniformData::update(Device *device) {
    auto &m = *this->m;
    for (auto &b: m.buffers)
        b.destroy();
    m.device = device;
    size_t n_frames = device->frames.size();
    m.buffers = vec<Buffer>(n_frames);
    for (int i = 0; i < n_frames; i++)
        m.buffers += Buffer {
            device, uint32_t(m.struct_sz), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
}

void UniformData::destroy() {
    auto &m = *this->m;
    size_t n_frames = m.device->frames.size();
    for (int i = 0; i < n_frames; i++)
        m.buffers[i].destroy();
}

/// called by the Render
void UniformData::transfer(size_t frame_id) {
    auto        &m = *this->m;
    auto   &device = *m.device;
    Buffer &buffer = m.buffers[frame_id]; /// todo -- UniformData never initialized (problem, that is)
    size_t      sz = buffer.sz;
    const char *data[2048];
    void* gpu;
    memset(data, 0, sizeof(data));
    m.fn(data);
    vkMapMemory(device, buffer, 0, sz, 0, &gpu);
    memcpy(gpu, data, sz);
    vkUnmapMemory(device, buffer);
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

VkShaderModule Device::module(std::filesystem::path p, Module type) {
    str key = p.string();
    assert(type != Compute);
    auto &m = type == Vertex ? v_modules : f_modules;
    if (!m.count(key)) {
        auto mc     = VkShaderModuleCreateInfo { };
        str code    = str::read_file(p);
        mc.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        mc.codeSize = code.length();
        mc.pCode    = reinterpret_cast<const uint32_t *>(code.cstr());
        assert (vkCreateShaderModule(device, &mc, nullptr, &m[key]) == VK_SUCCESS);
    }
    return m[key];
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

Device::Device(GPU &p_gpu, bool aa) {
    auto qcreate        = vec<VkDeviceQueueCreateInfo>(2);
    gpu                 = GPU(p_gpu);
    sampling            = VK_SAMPLE_COUNT_8_BIT; //aa ? gpu.support.max_sampling : VK_SAMPLE_COUNT_1_BIT;
    float priority      = 1.0f;
    uint32_t i_gfx      = gpu.index(GPU::Graphics);
    uint32_t i_present  = gpu.index(GPU::Present);
    qcreate            += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_gfx,     1, &priority };
    if (i_present != i_gfx)
        qcreate        += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_present, 1, &priority };
    auto features       = VkPhysicalDeviceFeatures { .samplerAnisotropy = aa ? VK_TRUE : VK_FALSE };
    vec<const char *> ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };
    VkDeviceCreateInfo ci {};
#ifndef NDEBUG
    //ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME; # not supported on macOS with molten-vk
    static const char *debug   = "VK_LAYER_KHRONOS_validation";
    ci.enabledLayerCount       = 1;
    ci.ppEnabledLayerNames     = &debug;
#endif
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = uint32_t(qcreate.size());
    ci.pQueueCreateInfos       = qcreate;
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = uint32_t(ext.size());
    ci.ppEnabledExtensionNames = ext;

    auto res = vkCreateDevice(gpu, &ci, nullptr, &device);
    assert(res == VK_SUCCESS);
    vkGetDeviceQueue(device, gpu.index(GPU::Graphics), 0, &queues[GPU::Graphics]); /// switch between like-queues if this is a hinderance
    vkGetDeviceQueue(device, gpu.index(GPU::Present),  0, &queues[GPU::Present]);
    
    /// create command pool (i think the pooling should be on each Frame)
    auto pool_info = VkCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = gpu.index(GPU::Graphics)
    };
    assert(vkCreateCommandPool(device, &pool_info, nullptr, &pool) == VK_SUCCESS);
}

/// this is to avoid doing explicit monkey-work, and keep code size down as well as find misbound texture
uint32_t Device::attachment_index(Texture *tx) {
    auto is_referencing = [&](Texture *a, Texture *b) {
        if (a->image && a->image == b->image)
            return true;
        if (a->view && a->view == b->view)
            return true;
        return false;
    };
    for (auto &f: frames)
        for (int i = 0; i < f.attachments.size(); i++)
            if (is_referencing(tx, &f.attachments[i]))
                return i;
    assert(false);
    return 0;
}

/// this is an initialize as well, and its also called when things change (resize or pipeline changes)
/// recreate pipeline may be a better name
void Device::update() {
    auto sz = vec2i { int(extent.width), int(extent.height) };
    uint32_t frame_count = frames.size();
    render.sync++;

    tx_color = ColorTexture { this, sz, null };
    tx_depth = DepthTexture { this, sz, null };
    ///
    /// thank god for asserts.
    /// stage should implicitly transition, we dont do this crap here:
    tx_color.set_stage(Texture::Stage::Transfer);
    tx_color.set_stage(Texture::Stage::Shader);
    tx_color.set_stage(Texture::Stage::Color);
    ///
    tx_depth.set_stage(Texture::Stage::Transfer);
    tx_depth.set_stage(Texture::Stage::Shader);
    tx_depth.set_stage(Texture::Stage::Color);
    ///
    /// hide usage, memory, aspect, mips; can infer this from context
    /// create VkCommandBuffer for pipelines across all frames
    /// create view, uniform buffer, render command
    for (size_t i = 0; i < frame_count; i++) {
        Frame &frame   = frames[i];
        frame.index    = i;
        auto   tx_swap = Texture {
            this, sz, swap_images[i], null,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false, VK_FORMAT_B8G8R8A8_SRGB, 1 };
        
        tx_swap.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // bit non-sense, but i believe it needs this association because the swap references
        // a layout without itsself being transitioned to this layout.  going with the flow for the now.
        // 'transitioning' to this layout does not work per the validator.
        // it simply needs an associative binding here.
        
        //tx_swap.set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        //tx_swap.set_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        //tx_swap.set_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        
        // look for attachments and verify its the same identity
        frame.attachments = vec<Texture> {
            tx_color, // a copy without the view is set on each one of these frames, when the view operator is called it creates the view
            tx_depth, // this may be actually creating it once.. darnit. test that.
            tx_swap
        };
        //if (!frame.ubo)
        //     frame.ubo = UniformBuffer<MVP>(this, &frame.uniform);
    }
    ///
    create_render_pass();
    
    tx_color.set_stage(Texture::Stage::Shader);
    tx_depth.set_stage(Texture::Stage::Shader);
    
    ///
    for (size_t i = 0; i < frame_count; i++) {
        Frame &frame   = frames[i];
        frame.update(); // r
    }
    /// render needs to update
    /// render.update();
}

void Device::initialize(Window *window) {
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
    ci.surface                    = *window;
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
    
    /// create swap-chain
    assert(vkCreateSwapchainKHR(device, &ci, nullptr, &swap_chain) == VK_SUCCESS);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, nullptr); /// the dreaded frame_count (2) vs image_count (3) is real.
    frames = vec<Frame>(frame_count, this); /// its a vector. its a size. its a value.
    
    /// get swap-chain images
    swap_images.resize(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format                        = surface_format.format;
    this->extent                  = extent;
    viewport                      = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    
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
    render             = Render(this);
    desc               = Descriptor(this);
    ///
    assert(vkCreateDescriptorPool(device, &dpi, nullptr, &desc.pool) == VK_SUCCESS);
    for (auto &f: frames)
        f.index = -1;
    update();
}

void Descriptor::destroy() {
    auto &device = *this->device;
    vkDestroyDescriptorPool(device, pool, nullptr);
}

#if 0
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

static VkFormat get_depth_format(VkPhysicalDevice gpu) {
    return supported_format(gpu,
        {VK_FORMAT_D32_SFLOAT,    VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
         VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
};
#endif

void Device::create_render_pass() {
    Frame &f0                     = frames[0];
    Texture &tx_ref               = f0.attachments[Frame::SwapView]; /// has msaa set to 1bit
    assert(f0.attachments.size() == 3);
    VkAttachmentReference    cref = tx_color; // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    dref = tx_depth; // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    rref = tx_ref;   // the odd thing is the associated COLOR_ATTACHMENT layout here;
    VkSubpassDescription     sp   = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, null, 1, &cref, &rref, &dref };

    VkSubpassDependency dep { };
    dep.srcSubpass                = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass                = 0;
    dep.srcStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask             = 0;
    dep.dstStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask             = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription adesc_clr_image  = f0.attachments[0];
    VkAttachmentDescription adesc_dep_image  = f0.attachments[1];
    VkAttachmentDescription adesc_swap_image = f0.attachments[2];
    
    std::array<VkAttachmentDescription, 3> attachments = {adesc_clr_image, adesc_dep_image, adesc_swap_image};
    VkRenderPassCreateInfo rpi { }; // VkAttachmentDescription needs to be 4, 4, 1
    rpi.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount           = uint32_t(attachments.size());
    rpi.pAttachments              = attachments.data();
    rpi.subpassCount              = 1;
    rpi.pSubpasses                = &sp;
    rpi.dependencyCount           = 1;
    rpi.pDependencies             = &dep;

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
    for (auto &[k,f]: f_modules)
        vkDestroyShaderModule(device, f, nullptr);
    for (auto &[k,v]: v_modules)
        vkDestroyShaderModule(device, v, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    desc.destroy();
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

