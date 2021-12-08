#include <vk/vk.hpp>

// attempt to get the verbose parts of vulkan marshalled into sanity with a reasonable Texture class
// auto-view creation, automatic layout controls, attachment/descriptor operators

void Texture::create_sampler() {
    Device &device = *this->device;
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(device, &props);
    ///
    VkSamplerCreateInfo si {};
    si.sType                     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter                 = VK_FILTER_LINEAR;
    si.minFilter                 = VK_FILTER_LINEAR;
    si.addressModeU              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeV              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeW              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.anisotropyEnable          = VK_TRUE;
    si.maxAnisotropy             = props.limits.maxSamplerAnisotropy;
    si.borderColor               = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    si.unnormalizedCoordinates   = VK_FALSE;
    si.compareEnable             = VK_FALSE;
    si.compareOp                 = VK_COMPARE_OP_ALWAYS;
    si.mipmapMode                = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.minLod                    = 0.0f;
    si.maxLod                    = float(mips);
    si.mipLodBias                = 0.0f;
    ///
    VkResult r = vkCreateSampler(device, &si, nullptr, &sampler);
    assert  (r == VK_SUCCESS);
}

/// see how the texture is created elsewhere with msaa
void Texture::create_resources() {
    Device &device = *this->device;
    VkImageCreateInfo imi {};
    ///
    layout                      = VK_IMAGE_LAYOUT_UNDEFINED;
    ///
    imi.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imi.imageType               = VK_IMAGE_TYPE_2D;
    imi.extent.width            = uint32_t(sz.x);
    imi.extent.height           = uint32_t(sz.y);
    imi.extent.depth            = 1;
    imi.mipLevels               = mips;
    imi.arrayLayers             = 1;
    imi.format                  = format;
    imi.tiling                  = tiling;
    imi.initialLayout           = layout;
    imi.usage                   = usage;
    imi.samples                 = ms ? device.sampling : VK_SAMPLE_COUNT_1_BIT;
    imi.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    ///
    assert(vkCreateImage(device, &imi, nullptr, &image) == VK_SUCCESS);
    ///
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, image, &req);
    VkMemoryAllocateInfo a {};
    a.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    a.allocationSize            = req.size;
    a.memoryTypeIndex           = device.memory_type(req.memoryTypeBits, mprops); /// this should work; they all use the same mprops from what i've seen so far and it stores that default
    assert(vkAllocateMemory(device, &a, nullptr, &memory) == VK_SUCCESS);
    vkBindImageMemory(device, image, memory, 0);
    ///
    if (ms)
        create_sampler();
    ///
    // run once like this:
    //set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); /// why?
}

struct LayoutMapping {
    VkImageLayout           from_layout;
    VkImageLayout           to_layout;
    uint64_t                srcAccessMask;
    uint64_t                dstAccessMask;
    VkPipelineStageFlagBits srcStage;
    VkPipelineStageFlagBits dstStage;
};

/// nvidia cares not about any of this
/// make access params, if needed.. it would be nice to
/// tuck anything access-related into implicit behaviours
/// this is actual vulkan hell at the moment.
/// i need to find some specs for what is transferrable to what,
/// then get rid of explicit steps to get there from user code.
///
void Texture::set_layout(VkImageLayout next_layout)
{
    if (next_layout == layout)
        return;
    Device       &device = *this->device;
    static auto mappings = vec<LayoutMapping> {
        LayoutMapping {
            VK_IMAGE_LAYOUT_UNDEFINED,                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,                                          VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,          VK_PIPELINE_STAGE_TRANSFER_BIT
        },
        LayoutMapping {
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,               VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        },
        LayoutMapping {
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,                  VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      VK_PIPELINE_STAGE_TRANSFER_BIT
        },
        LayoutMapping {
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        },
        LayoutMapping {
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        },
        
        LayoutMapping {
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        },
        
        
        
        LayoutMapping {
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,               VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        },
        
        LayoutMapping {
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,               VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        }
    };
    ///
    for (LayoutMapping &t: mappings)
        if (t.from_layout == layout && t.to_layout == next_layout) {
            VkCommandBuffer      cb                 = device.begin();
            ///
            VkImageMemoryBarrier barrier {};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = layout;
            barrier.newLayout                       = next_layout;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = image;
            barrier.subresourceRange.aspectMask     = aflags;//VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = mips;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = t.srcAccessMask;
            barrier.dstAccessMask                   = t.dstAccessMask;
            ///
            vkCmdPipelineBarrier( /// ! this must be the level at which the validation works, sort of a proxy in the command buffer stream
                cb,
                t.srcStage, t.dstStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            device.submit(cb);
            layout = next_layout;
            return;
        }
    ///
    assert(false);
}

Texture::operator bool()  { return image != VK_NULL_HANDLE; }
bool Texture::operator!() { return image == VK_NULL_HANDLE; }

int Texture::auto_mips(uint32_t mips, vec2i sz) {
    return 1; /// mips == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : std::max(1, mips);
}

/// its fun hiding verbose parts of the api while keeping it dynamic
Texture::operator VkAttachmentReference() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    //bool     is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t    index = device->attachment_index(this);
    
    ///
    /// why oh why dont we just read from the layout direct here?
    auto   attach_ref = VkAttachmentReference {
        .attachment = index,  // trying this, step through it.
        .layout     = layout  //is_color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                              //   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    return attach_ref;
}

Texture::operator VkAttachmentDescription() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto     desc = VkAttachmentDescription {
        .format         = format,
        .samples        = ms ? device->sampling : VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = image_ref ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                      VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = is_color ?  VK_ATTACHMENT_STORE_OP_STORE :
                                      VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = image_ref ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                          is_color  ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    return desc;
}

void Texture::transfer_pixels(rgba *pixels) { /// sz = 0, eh.
    Device &device = *this->device;
    ///
    create_resources();
    if (pixels) {
        auto    nbytes = VkDeviceSize(sz.x * sz.y * 4); /// adjust bytes if format isnt rgba; implement grayscale
        Buffer staging = Buffer(&device, nbytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void* data = null;
        vkMapMemory(device, staging, 0, nbytes, 0, &data);
        memcpy(data, pixels, size_t(nbytes));
        vkUnmapMemory(device, staging);
        
        staging.copy_to(this);
        staging.destroy();
    }
    ///if (mip > 0) # skipping this for now
    ///    generate_mipmaps(device, image, format, sz.x, sz.y, mip);
}

VkImageView Texture::create_view(VkDevice device, vec2i &sz, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageView  view;
    uint32_t      mips = auto_mips(mip_levels, sz);
    auto             v = VkImageViewCreateInfo {};
    v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    v.image            = image;
    v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    v.format           = format;
    v.subresourceRange = { aspect_flags, 0, mips, 0, 1 };
    assert (vkCreateImageView(device, &v, nullptr, &view) == VK_SUCCESS);
    return view;
}

/// create with solid color
Texture::Texture(Device *device, vec2i sz, rgba clr,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms,
                 VkFormat format, int mips) :
                    device(device), sz(sz),
                    mips(auto_mips(mips, sz)), mprops(m),
                    ms(ms), format(format), usage(u), aflags(a) {
    //rgba *px = (rgba *)malloc(sz.x * sz.y * 4);
    //for (int i = 0; i < sz.x * sz.y; i++)
    //    px[i] = clr;
    transfer_pixels(null);
    //free(px);
    image_ref = false;
}

/// create with image (rgba required)
Texture::Texture(Device *device, Image &im,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms,
                 VkFormat format, int mips) :
                    device(device), sz(im.size()), mips(auto_mips(mips, sz)), mprops(m), ms(ms), format(format), aflags(a)  {
    rgba *px = &im.pixel<rgba>(0, 0);
    transfer_pixels(px);
    free(px);
    image_ref = false;
}

Texture::Texture(Device *device, vec2i sz, VkImage image, VkImageView view, VkImageUsageFlags u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms, VkFormat format, int mips):
                    device(device), image(image), view(view),
                    sz(sz), mips(auto_mips(mips, sz)), mprops(m), ms(ms), image_ref(true), format(format), usage(u), aflags(a) { }

Texture::operator VkImageView &() {
    if (!view)
         view = create_view(*device, sz, image, format, aflags, mips); /// hide away the views, only create when they are used
    return view;
}

Texture::operator VkImage &() {
    return image;
}

Texture::operator VkDeviceMemory &() {
    return memory;
}

void Texture::destroy() {
    if (device) {
        vkDestroyImageView(*device, view,   nullptr);
        vkDestroyImage(*device,     image,  nullptr);
        vkFreeMemory(*device,       memory, nullptr);
    }
}

