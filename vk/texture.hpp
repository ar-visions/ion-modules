#pragma once
#include <vk/vk.hpp>

struct Texture {
    ///
    struct Stage {
        ///
        enum Type {
            Undefined,
            Transfer,
            Shader,
            Color,
            Depth
        };
        Type value;
        ///
        struct Data {
            enum Type               value  = Undefined; /// redundant is good. go redundant. somewhat.
            VkImageLayout           layout = VK_IMAGE_LAYOUT_UNDEFINED;
            uint64_t                access = 0;
            VkPipelineStageFlagBits stage;
            inline bool operator==(Data &b) {
                return layout == b.layout;
            }
        };
        ///
        static const Data types[5];
        const Data & data() {
            return types[value];
        }
        ///
        inline bool operator==(Stage &b)      { return value == b.value; }
        inline bool operator!=(Stage &b)      { return value != b.value; }
        inline bool operator==(Stage::Type b) { return value == b; }
        inline bool operator!=(Stage::Type b) { return value != b; }
        ///
        Stage(      Type    type = Undefined);
        Stage(const Stage & ref) : value(ref.value) { };
        Stage(      Stage & ref) : value(ref.value) { };
        ///
        bool operator>(Stage &b) { return value > b.value; }
        bool operator<(Stage &b) { return value < b.value; }
        operator Type() {
            return value;
        }
    };
    
    struct Data {
        Device                 *device     = null;
        VkImage                 image      = VK_NULL_HANDLE;
        VkImageView             view       = VK_NULL_HANDLE;
        VkDeviceMemory          memory     = VK_NULL_HANDLE;
        VkDescriptorImageInfo   info       = {};
        VkSampler               sampler    = VK_NULL_HANDLE;
        vec2i                   sz         = { 0, 0 };
        int                     mips       = 0;
        VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkImageTiling           tiling     = VK_IMAGE_TILING_OPTIMAL; /// [not likely to be a parameter]
        bool                    ms         = false;
        bool                    image_ref  = false;
        VkFormat                format;
        VkImageUsageFlags       usage;
        VkImageAspectFlags      aflags;
        Stage                   stage;
        VkImageLayout           layout_override = VK_IMAGE_LAYOUT_UNDEFINED;
        
        ///
        void set_stage(Stage new_layout);
        static int auto_mips(uint32_t, vec2i);
        static VkImageView create_view(VkDevice, vec2i &, VkImage, VkFormat, VkImageAspectFlags, uint32_t);
        ///

        operator    bool();
        bool        operator!();
        operator    VkImage &();
        operator    VkImageView &();
        operator    VkDeviceMemory &();
        operator    VkAttachmentDescription();
        operator    VkAttachmentReference();
        operator    VkDescriptorImageInfo &();
        void        destroy();
        VkImageView image_view();
        VkSampler   image_sampler();
        void        create_sampler();
        void        create_resources();
        VkWriteDescriptorSet write_desc(VkDescriptorSet &ds);
        
        Data(nullptr_t n = null) { }
        Data(Device *, vec2i, rgba,
             VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Data(Device *device, Image &im,
             VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Data(Device *device, vec2i sz, VkImage image, VkImageView view,
             VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    };
    ///
    std::shared_ptr<Data> data;
    
    Texture(nullptr_t n = null) { }
    Texture(Device *device, vec2i sz, rgba clr,
            VkImageUsageFlags usage, VkMemoryPropertyFlags memory, VkImageAspectFlags aspect, bool image_ref,
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1) :
        data(std::shared_ptr<Data>(
            new Data { device, sz, clr, usage, memory, aspect, image_ref, format, mips }
        ))
    {
        //rgba *px = (rgba *)malloc(sz.x * sz.y * 4);
        //for (int i = 0; i < sz.x * sz.y; i++)
        //    px[i] = clr;
        transfer_pixels(null);
        //free(px);
        image_ref = false;
    }

    Texture(Device *device, Image &im,
            VkImageUsageFlags usage, VkMemoryPropertyFlags memory, VkImageAspectFlags aspect, bool image_ref,
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1) :
        data(std::shared_ptr<Data>(
            new Data { device, im, usage, memory, aspect, image_ref, format, mips }
        ))
    {
        rgba *px = &im.pixel<rgba>(0, 0);
        transfer_pixels(px);
        free(px);
        data->image_ref = false;
    }

    Texture(Device *device, vec2i sz, VkImage image, VkImageView view,
            VkImageUsageFlags usage, VkMemoryPropertyFlags memory, VkImageAspectFlags aspect, bool image_ref,
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1) :
        data(std::shared_ptr<Data>(
            new Data { device, sz, image, view, usage, memory, aspect, image_ref, format, mips }
        )) { }
    
    void transfer_pixels(rgba *pixels);
    void set_stage(Stage n)            { return data->set_stage(n); }
    
    /// pass-through operators
    operator  bool()                   { return    data &&  *data; }
    bool operator!()                   { return  !data  || !*data; }
    operator VkImage &()               { return   *data; }
    operator VkImageView &()           { return   *data; }
    operator VkDeviceMemory &()        { return   *data; }
    operator VkAttachmentReference()   { return   *data; }
    operator VkAttachmentDescription() { return   *data; }
    operator VkDescriptorImageInfo &() { return   *data; }
    
    /// pass-through methods
    void destroy() { return data->destroy(); };
    VkWriteDescriptorSet write_desc(VkDescriptorSet &ds) { return data->write_desc(ds); }
};

struct ColorTexture:Texture {
    ColorTexture(Device *device, vec2i sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_SRGB, 1) {
            data->set_stage(Stage::Color);
        }
};

/*
return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
);

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
*/

struct DepthTexture:Texture {
    DepthTexture(Device *device, vec2i sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_DEPTH_BIT, true, VK_FORMAT_D32_SFLOAT, 1) {
            data->set_stage(Stage::Depth);
        }
};

struct SwapImage:Texture {
    SwapImage(Device *device, vec2i sz, VkImage image):
        Texture(device, sz, image, null,
             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_B8G8R8A8_SRGB, 1) {
            data->layout_override = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
};
