#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>

///
/// a good vulkan api will be fully accessible but minimally interfaced
/// for instance for every image, you need all of this crap, so why on earth
/// are they separate types?  if there is no rationale to keep them separate, you
/// combine. they are all still there and cast safely from the object.
struct Texture {
    protected:
    VkSampler create_sampler();
    void create_resources(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits numSamples,
                          VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory, VkSampler *sampler);
    
    Device        &device;
    VkImage        image      = VK_NULL_HANDLE;
    VkImageView    view       = VK_NULL_HANDLE;
    VkDeviceMemory memory     = VK_NULL_HANDLE;
    VkSampler      sampler    = VK_NULL_HANDLE;
    VkFormat       format;
    vec2i          sz         = { 0, 0 };
    int            mip_levels = 0;
    ///
    void           destroy();
    operator       VkImageView();
    
    void transition_layout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mip_levels);

    Texture(vec2i size, rgba clr, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mip_levels = -1);
    Texture(Image &im, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mip_levels = -1);
    
    operator  bool();
    bool operator!();
};
