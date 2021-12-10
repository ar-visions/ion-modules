#pragma once
#include <vk/vk.hpp>

struct Texture {
    Device                 *device     = null;
    VkImage                 image      = VK_NULL_HANDLE;
    VkImageView             view       = VK_NULL_HANDLE;
    VkDeviceMemory          memory     = VK_NULL_HANDLE;
    VkDescriptorImageInfo   info       = {};
    VkSampler               sampler    = VK_NULL_HANDLE;
    vec2i                   sz         = { 0, 0 };
    int                     mips       = 0;
    VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageLayout           layout     = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageTiling           tiling     = VK_IMAGE_TILING_OPTIMAL; /// [not likely to be a parameter]
    bool                    ms         = false;
    bool                    image_ref  = false;
    VkFormat                format;
    VkImageUsageFlags       usage;
    VkImageAspectFlags      aflags;
    
    /// this specific pattern should be replicated elsewhere.  Its a type, its an enum, its data associated.
    /// the enum makes it ambiguous with null and that is a feature not a bug my good lad
    /// constructed with Undefined (which is defined), single 1 unit in register form
    struct Stage {
        ///
        enum Type {
            Undefined,
            Transfer,
            Shader,
            Color,
            Depth
        };
        ///
        struct Data {
            enum Type               value;
            VkImageLayout           layout;
            uint64_t                access;
            VkPipelineStageFlagBits stage;
            inline bool operator==(Data &b) {
                return layout == b.layout;
            }
        };
        ///
        static const Data types[5];
        Data            & type;
        ///
        inline bool operator==(Stage &b)      { return type.value == b.type.value; }
        inline bool operator!=(Stage &b)      { return type.value != b.type.value; }
        inline bool operator==(Stage::Type b) { return type.value == b; }
        inline bool operator!=(Stage::Type b) { return type.value != b; }
        ///
        Stage(      Type    type = Undefined);
        Stage(const Stage & ref) : type(ref.type) { };
        Stage(      Stage & ref) : type(ref.type) { };
        ///
        bool operator>(Stage &b) { return type.value > b.type.value; }
        bool operator<(Stage &b) { return type.value < b.type.value; }
        Stage &operator=(const Stage &ref);
    };
    Stage stage;
    
    ///
    void set_stage(Stage new_layout);
    static int auto_mips(uint32_t, vec2i);
    static VkImageView create_view(VkDevice, vec2i &, VkImage, VkFormat, VkImageAspectFlags, uint32_t);
    ///
    operator VkDescriptorImageInfo &() {
        info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = view,
            .sampler     = sampler
        };
        return info;
    }

    VkWriteDescriptorSet write_desc(VkDescriptorSet &ds) {
        VkDescriptorImageInfo &info = *this;
        return {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = ds,
            .dstBinding      = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo      = &info
        };
    }

    Texture(nullptr_t n = null) { }
    Texture(Device *, vec2i, rgba,
            VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    Texture(Device *device, Image &im,
            VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    Texture(Device *device, vec2i sz, VkImage image, VkImageView view,
            VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    ///
    operator VkDescriptorImageInfo() {
        return { sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    ///
    operator  bool();
    bool operator!();
    operator VkImage &();
    operator VkImageView &();
    operator VkDeviceMemory &();
    operator VkAttachmentReference();
    operator VkAttachmentDescription();
    
protected:
    void transfer_pixels(rgba *pixels);
    void create_sampler();
    void create_resources();
    void destroy();
};

struct ColorTexture:Texture {
    ColorTexture(Device *device, vec2i sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_SRGB, 1) { }
};

struct DepthTexture:Texture {
    DepthTexture(Device *device, vec2i sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_SRGB, 1) { }
};
