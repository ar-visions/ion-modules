#pragma once
#include <vk/vk.hpp>
#include <vk/opaque.hpp>

/// its just enums for different resources, texture is a basic resource
struct Asset {
    /// the general shader supports reflection, when its actually used see.
    enum Type { // i think turn this into the sane thing, but, only after it works.  not getting lost for an additional day because of a flag ordinal cross-up party
        Undefined = 0,
        Color     = 1,
        Mask      = 2, // todo: [4] important to keep this as a generic; it can be by-passed, combined, or loaded pre-baked in certain routines, and not in others (a spray/brush interface for example)
        Normal    = 3,
        Shine     = 4, // it would basically have AO or other
        Rough     = 5,
        Displace  = 6, // displace offset from the normal map if combined, its just a relative magnitude vert at the normal dir
        Max       = Displace
    };
    
    /// this one we cannot have, i mean only if it was given a pointer to it.
    typedef FlagsOf<Type> Types;
    static void descriptor(Type t, VkDescriptorSetLayoutBinding &desc);
};

struct iTexture;
///
struct Texture:Asset {
    struct Stage {
        enum Type {
            Undefined,
            Transfer,
            Shader,
            Color,
            Depth
        };
        ///
        Type value;
        struct Data {
            enum Type               value  = Undefined;
            VkImageLayout           layout = VK_IMAGE_LAYOUT_UNDEFINED;
            uint64_t                access = 0;
            VkPipelineStageFlagBits stage;
            inline bool operator==(Data &b) { return layout == b.layout; }
        };
        ///
        static const Data types[5];
        const Data & data() { return types[value]; }
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
             operator     Type() { return value; }
    };
    
    ///
    sh<iTexture> intern;
    
    void transfer(VkCommandBuffer &, VkBuffer &);
    
    iTexture *get_data();
    
    Texture(std::nullptr_t n = null);
    Texture(Device            *dev,     vec2i sz,             rgba clr,
            VkImageUsageFlags  usage,   VkMemoryPropertyFlags memory,
            VkImageAspectFlags aspect,  bool                  ms,
            VkFormat           format = VK_FORMAT_R8G8B8A8_UNORM, int mips = -1, Image lazy = null);
    Texture(Device            *dev,  Image &im, Asset::Type t);
    Texture(Device            *dev,  vec2i                 sz,
            VkImage            image,  VkImageView           view,
            VkImageUsageFlags  usage,  VkMemoryPropertyFlags memory,
            VkImageAspectFlags aspect, bool                  ms,
            VkFormat           format = VK_FORMAT_R8G8B8A8_UNORM, int mips = -1);
    
    Texture &operator=(const Texture &ref);
    void set_stage(Stage s);
    void push_stage(Stage s);
    void pop_stage();
    void transfer_pixels(rgba *pixels);
    
public:
    /// pass-through operators
    operator  bool                    ();
    bool operator!                    ();
    bool operator==        (Texture &tx);
    operator VkImage                 &();
    operator VkImageView             &();
    operator VkDeviceMemory          &();
    operator VkAttachmentReference   &();
    operator VkAttachmentDescription &();
    operator VkDescriptorImageInfo   &();
    
    void destroy();
    VkWriteDescriptorSet descriptor(VkDescriptorSet &ds, uint32_t binding);
};

typedef map<Asset::Type, Texture *> Assets;

struct ColorTexture:Texture { ColorTexture(Device *dev, vec2i sz, rgba clr);   };
struct DepthTexture:Texture { DepthTexture(Device *dev, vec2i sz, rgba clr);   };
struct SwapImage:Texture    { SwapImage(Device *dev, vec2i sz, VkImage image); };

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


