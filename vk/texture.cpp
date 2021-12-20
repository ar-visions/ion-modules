#include <vk/vk.hpp>

/// create with solid color
Texture::Data::Data(Device *device, vec2i sz, rgba clr,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms,
                 VkFormat format, int mips) :
                    device(device), sz(sz),
                    mips(auto_mips(mips, sz)), mprops(m),
                    ms(ms), format(format), usage(u), aflags(a) { }

/// create with image (rgba required)
Texture::Data::Data(Device *device, Image &im,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms,
                    VkFormat           f, int  mips) :
                       device(device), sz(im.size()), mips(auto_mips(mips, sz)),
                       mprops(m),      ms(ms),        format(f),     aflags(a)  { }

Texture::Data::Data(Device *device, vec2i sz, VkImage image, VkImageView view, VkImageUsageFlags u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms, VkFormat format, int mips):
                       device(device), image(image),              view(view),
                       sz(sz),         mips(auto_mips(mips, sz)), mprops(m),
                       ms(ms),         image_ref(true),           format(format),
                       usage(u),       aflags(a) { }

Texture::Stage::Stage(Stage::Type value) : value(value) { }

const Texture::Stage::Data Texture::Stage::types[5] = {
    { Texture::Stage::Undefined, VK_IMAGE_LAYOUT_UNDEFINED,                0,                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { Texture::Stage::Transfer,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
    { Texture::Stage::Shader,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  },
    { Texture::Stage::Color,     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { Texture::Stage::Depth,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
};

void Texture::Data::create_sampler() {
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
void Texture::Data::create_resources() {
    Device &device = *this->device;
    VkImageCreateInfo imi {};
    ///
    layout_override             = VK_IMAGE_LAYOUT_UNDEFINED;
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
    imi.initialLayout           = layout_override;
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

void Texture::Data::pop_stage() {
    assert(stage.size() >= 2);
    /// kind of need to update it.
    stage.pop();
    set_stage(stage.top());
}

void Texture::Data::set_stage(Stage next_stage) {
    Device  & device = *this->device;
    Stage       cur  = prv_stage;
    while (cur != next_stage) {
        Stage  from  = cur;
        Stage  to    = cur;
        ///
        /// skip over that color thing because this is quirky
        do to = Stage::Type(int(to) + ((next_stage > prv_stage) ? 1 : -1));
        while (to == Stage::Color && to != next_stage);
        ///
        /// transition through the image membrane, as if we were insane in-the
        Texture::Stage::Data      data = from.data();
        Texture::Stage::Data next_data =   to.data();
        VkImageMemoryBarrier   barrier = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout                       =      data.layout,
            .newLayout                       = next_data.layout,
            .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .image                           = image,
            .subresourceRange.aspectMask     = aflags,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = uint32_t(mips),
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
            .srcAccessMask                   = VkAccessFlags(     data.access),
            .dstAccessMask                   = VkAccessFlags(next_data.access)
        };
        VkCommandBuffer cb = device.begin();
        vkCmdPipelineBarrier(
            cb, data.stage, next_data.stage, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);
        device.submit(cb);
        cur = to;
    }
    prv_stage = next_stage;
}

void Texture::Data::push_stage(Stage next_stage) {
    if (stage.size() == 0)
        stage.push(Stage::Undefined); /// default stage, when pop'd
    if (next_stage == stage.top())
        return;
    
    set_stage(next_stage);
    stage.push(next_stage);
}

void Texture::transfer_pixels(rgba *pixels) { /// sz = 0, eh.
    Data &d = *data;
    Device &device = *d.device;
    ///
    d.create_resources();
    if (pixels) {
        auto    nbytes = VkDeviceSize(d.sz.x * d.sz.y * 4); /// adjust bytes if format isnt rgba; implement grayscale
        Buffer staging = Buffer(&device, nbytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        ///
        void* mem = null;
        vkMapMemory(device, staging, 0, nbytes, 0, &mem);
        memcpy(mem, pixels, size_t(nbytes));
        vkUnmapMemory(device, staging);
        ///
        push_stage(Stage::Transfer);
        staging.copy_to(this);
        //pop_stage(); // ah ha!..
        staging.destroy();
    }
    ///if (mip > 0) # skipping this for now
    ///    generate_mipmaps(device, image, format, sz.x, sz.y, mip);
}

VkImageView Texture::Data::create_view(VkDevice device, vec2i &sz, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageView  view;
    uint32_t      mips = auto_mips(mip_levels, sz);
    //auto         usage = VkImageViewUsageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, null, VK_IMAGE_USAGE_SAMPLED_BIT };
    auto             v = VkImageViewCreateInfo {};
    v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  //v.pNext            = &usage;
    v.image            = image;
    v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    v.format           = format;
    v.subresourceRange = { aspect_flags, 0, mips, 0, 1 };
    assert (vkCreateImageView(device, &v, nullptr, &view) == VK_SUCCESS);
    return view;
}

int Texture::Data::auto_mips(uint32_t mips, vec2i sz) {
    return 1; /// mips == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : std::max(1, mips);
}

Texture::Data::operator bool () { return image != VK_NULL_HANDLE; }
bool Texture::Data::operator!() { return image == VK_NULL_HANDLE; }

Texture::Data::operator VkImage &() {
    return image;
}

Texture::Data::operator VkImageView &() {
    if (!view)
         view = create_view(device_handle(device), sz, image, format, aflags, mips); /// hide away the views, only create when they are used
    return view;
}

Texture::Data::operator VkDeviceMemory &() {
    return memory;
}

Texture::Data::operator VkAttachmentDescription() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto     desc = VkAttachmentDescription {
        .format         = format,
        .samples        = ms ? device->sampling : VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = image_ref ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                      VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        =  is_color ? VK_ATTACHMENT_STORE_OP_STORE :
                                      VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        /// odd thing to need to pass the life cycle like this
        .finalLayout    = image_ref  ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                           is_color  ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    return desc;
}

Texture::Data::operator VkAttachmentReference() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  //bool  is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t index = device->attachment_index(this);
    return VkAttachmentReference {
        .attachment  = index,
        .layout      = (layout_override != VK_IMAGE_LAYOUT_UNDEFINED) ? layout_override :
                       (stage.size() ? stage.top().data().layout : VK_IMAGE_LAYOUT_UNDEFINED)
    };
}

VkImageView Texture::Data::image_view() {
    if (!view)
         view = create_view(*device, sz, image, format, aflags, mips);
    return view;
}

VkSampler Texture::Data::image_sampler() {
    if (!sampler)
         create_sampler();
    return sampler;
}

Texture::Data::operator VkDescriptorImageInfo &() {
    info  = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView   = image_view(),
        .sampler     = image_sampler()
    };
    return info;
}

///
void Texture::Data::destroy() {
    if (device) {
        vkDestroyImageView(*device, view,   nullptr);
        vkDestroyImage(*device,     image,  nullptr);
        vkFreeMemory(*device,       memory, nullptr);
    }
}

VkWriteDescriptorSet Texture::Data::write_desc(VkDescriptorSet &ds) {
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


