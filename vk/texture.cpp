#include <vk/vk.hpp>

// attempt to get the verbose parts of vulkan marshalled into sanity with a reasonable Texture class
// auto-view creation, automatic layout controls, attachment/descriptor operators

static void
generate_mipmaps(Device &device, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device, imageFormat, &formatProperties);

    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer commandBuffer = device.begin();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    device.submit(commandBuffer);
}

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
    si.maxLod                    = float(get_mips(mip_levels, sz));
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
    imi.mipLevels               = get_mips(mip_levels, sz);
    imi.arrayLayers             = 1;
    imi.format                  = format;
    imi.tiling                  = tiling;
    imi.initialLayout           = layout;
    imi.usage                   = usage;
    imi.samples                 = VK_SAMPLE_COUNT_1_BIT; // msaa: todo
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
    if (msaa != VK_SAMPLE_COUNT_1_BIT)
        create_sampler();
    set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
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
void Texture::set_layout(VkImageLayout next_layout)
{
    if (next_layout == layout)
        return;
    Device       &device = *this->device;
    uint32_t        mips = get_mips(mip_levels, sz);
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
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = mips;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = t.srcAccessMask;
            barrier.dstAccessMask                   = t.dstAccessMask;
            ///
            vkCmdPipelineBarrier(
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

int Texture::get_mips(int mip_levels, vec2i sz) {
    return mip_levels == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : std::max(1, mip_levels);
}

/// its fun hiding verbose parts of the api while keeping it dynamic
Texture::operator VkAttachmentReference() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool     is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    int         index = device->attachment_index(this);
    VkAttachmentReference ref = {
        .attachment = index,
        .layout     = is_color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    };
}

Texture::operator VkAttachmentDescription() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkAttachmentDescription ret = {
        .format         = format,
        .samples        = msaa,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = is_color ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    };
    return ret;
}

void Texture::create(rgba *pixels) { /// sz = 0, eh.
    Device &device = *this->device;
    auto    nbytes = VkDeviceSize(sz.x * sz.y * 4); /// adjust bytes if format isnt rgba; implement grayscale
    auto    mip    = get_mips(mip_levels, sz);
    assert(pixels);
    ///
    Buffer staging = Buffer(&device, nbytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); /// make sure this is the same
    void* data = null;
    vkMapMemory(device, staging, 0, nbytes, 0, &data);
    memcpy(data, pixels, size_t(nbytes));
    vkUnmapMemory(device, staging);
    create_resources();
    staging.copy_to(this);
    staging.destroy();
    ///
    if (mip > 0)
        generate_mipmaps(device, image, format, sz.x, sz.y, mip);
}

VkImageView Texture::create_view(VkDevice device, vec2i &sz, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageView  view;
    uint32_t      mips = get_mips(mip_levels, sz);
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
                 VkImageAspectFlags a, VkSampleCountFlagBits s,
                 VkFormat format, int mip_levels) :
                    device(device), format(format), sz(sz),
                    mip_levels(mip_levels), mprops(m), usage(u),
                    aflags(a), msaa(s) {
    rgba *px = (rgba *)malloc(sz.x * sz.y * 4);
    for (int i = 0; i < sz.x * sz.y; i++)
        px[i] = clr;
    create(px);
    free(px);
}

/// create with image (rgba required)
Texture::Texture(Device *device, Image &im,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, VkSampleCountFlagBits s,
                 VkFormat format, int mip_levels) :
                    device(device), format(format), sz(im.size()), mip_levels(mip_levels), mprops(m), aflags(a), msaa(s)  {
    rgba *px = &im.pixel<rgba>(0, 0);
    create(px);
    free(px);
}

Texture::Texture(Device *device, vec2i sz, VkImage image, VkImageView view, VkImageUsageFlags u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, VkSampleCountFlagBits s, VkFormat format, int mip_levels):
                    device(device), image(image), view(view), format(format), sz(sz),
                    mip_levels(mip_levels), mprops(m), usage(u), aflags(a), msaa(s) { }

Texture::operator VkImageView &() {
    if (!view)
         view = create_view(*device, sz, image, format, aspect, mip_levels); /// hide away the views, only create when they are used
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

