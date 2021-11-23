#include <vk/texture.hpp>

static void
generate_mipmaps(Device &device, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device, imageFormat, &formatProperties);

    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

    endSingleTimeCommands(commandBuffer);
}

VkSampler create_sampler() {
    VkSampler                   sampler;
    VkPhysicalDeviceProperties  props {};
    vkGetPhysicalDeviceProperties(gpu, &props);

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
    si.maxLod                    = float(mip_levels);
    si.mipLodBias                = 0.0f;

    VkResult r = vkCreateSampler(device, &si, nullptr, &sampler);
    assert  (r == VK_SUCCESS);
    return sampler;
}

void create_resources(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits numSamples,
                      VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory,
                      VkSampler *sampler)
{
    VkImageCreateInfo imi {};
    imi.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imi.imageType               = VK_IMAGE_TYPE_2D;
    imi.extent.width            = width;
    imi.extent.height           = height;
    imi.extent.depth            = 1;
    imi.mipLevels               = mip_levels;
    imi.arrayLayers             = 1;
    imi.format                  = format;
    imi.tiling                  = tiling;
    imi.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    imi.usage                   = usage;
    imi.samples                 = numSamples;
    imi.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;

    assert(vkCreateImage(device, &imi, nullptr, image) == VK_SUCCESS);
    
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, *image, &req);
    VkMemoryAllocateInfo a {};
    a.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    a.allocationSize            = req.size;
    a.memoryTypeIndex           = findMemoryType(req.memoryTypeBits, properties);
    
    assert(vkAllocateMemory(device, &a, nullptr, imageMemory) == VK_SUCCESS);
    vkBindImageMemory(device, *image, *imageMemory, 0);
    
    if (sampler)
        *sampler = create_sampler();
}

struct LayoutMapping {
    VkImageLayout           oldLayout;
    VkImageLayout           newLayout;
    VkAccessFlagBits        srcAccessMask;
    VkAccessFlagBits        dstAccessMask;
    VkPipelineStageFlagBits srcStage;
    VkPipelineStageFlagBits dstStage;
};

void Texture::transition_layout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mip_levels)
{
    static vec<LayoutMapping> mappings = {
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
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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
        if (oldLayout == t.oldLayout && newLayout == t.newLayout) {
            VkCommandBuffer      cb                 = device.begin();
            ///
            VkImageMemoryBarrier barrier {};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = oldLayout;
            barrier.newLayout                       = newLayout;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = image;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = mip_levels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = t.srcAccessMask;
            barrier.dstAccessMask                   = t.dstAccessMask;
            ///
            VkPipelineStageFlags sourceStage        = t.sourceStage;
            VkPipelineStageFlags destinationStage   = t.destinationStage;
            ///
            /// can easily become method calls on a possible Command object from begin
            /// until i see a need for that i dont want to add anything else
            vkCmdPipelineBarrier(
                cb,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            device.submit(cb);
            return true;
        }
    ///
    assert(false);
    return false;
}

Texture::operator bool()  { return image != VK_NULL_HANDLE; }
bool Texture::operator!() { return image == VK_NULL_HANDLE; }

Texture::Texture(vec2i sz, rgba clr, VkFormat format, int mip_levels) : sz(sz) {
    Device &device = Vulkan::device();
    rgba   *pixels = (rgba *)malloc(sz.x * sz.y * 4);
    auto    nbytes = VkDeviceSize(sz.x * sz.y * 4);
    mip            = mip_levels == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : mip_levels;

    assert(pixels);
    for (int i = 0; i < sz; i++)
        ((rgba *)pixels)[i] = clr;
    
    Buffer staging = Buffer(nbytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* data = null;
    vkMapMemory(device, staging, 0, nbytes, 0, &data);
    memcpy(data, pixels, size_t(nbytes));
    vkUnmapMemory(device, staging);
    free(pixels);

    /// design: we'll create our own sampler in special cases and use device default
    create_resources(size.x, size.y, mip,
                     VK_SAMPLE_COUNT_1_BIT,
                     format, VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     &image, &memory, &sampler);

    /// needs param for format
    transitionImageLayout(image, format,
                     VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     mip_levels);
    
    copyBufferToImage(stagingBuffer, textureImage, uint32_t(texWidth), uint32_t(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    staging.destroy();

    if (mip > 0)
        generate_mipmaps(device, image, VK_FORMAT_R8G8B8A8_SRGB, size.x, size.y, mip);
}

Texture::Texture(vec2i size, rgba clr, int mip_levels) {
}

Texture::Texture(Image &im, int mip_levels) {
}

Texture::operator VkImageView() { // vulkan was clearly made with C++ in mind
    return view;
}

Texture::operator VkImage() {
    return image;
}

Texture::operator VkDeviceMemory() {
    return memory;
}

void Texture::destroy() {
    vkDestroyImageView(device, view,   nullptr);
    vkDestroyImage(device,     image,  nullptr);
    vkFreeMemory(device,       memory, nullptr);
}