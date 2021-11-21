#pragma once
#include <data/data.hpp>
#include <vk/vk.hpp>

struct Framebuffer {
    VkImage         image;
    VkImageView     view;
    VkFramebuffer   framebuffers;
    Buffer          uniform;
    void destroy(VkDevice &device);
};

///
/// a good vulkan api will be fully accessible but minimally interfaced
struct Device {
protected:
    // combine all of this crap since its 1:1 frame ops
    void create_swapchain();
    void create_command_buffers();
    void create_render_pass();
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    // we want to pslit off as much as we can to 'pipeline', including shader for now
    
    VkSampleCountFlagBits      msaa_samples;
    VkRenderPass               render_pass;
    VkDescriptorSetLayout      ds_layout;
    VkDescriptorPool           desc_pool;
    VkPipelineLayout           pipeline_layout;
    VkPipeline                 pipeline;
    vec<VkDescriptorSet>       desc_sets;
    vec<VkCommandBuffer>       command_buffers;
    
    // throw it all in the gumbo.

    GPU                       &gpu;
    VkCommandPool              pool;
    VkDevice                   device;
    VkQueue                    queues[GPU::Complete];
    
    /// important that these are here until it makes complete to split them off
    vec<VkImage>               swap_images;
    VkSwapchainKHR             swap_chain;
    VkFormat                   format;
    VkExtent2D                 extent;
    
    /// lets combine these into a singular data structure
    vec<Framebuffer>           framebuffers;
    
    std::vector<VkImage>       images;
    std::vector<VkImageView>   views;
    std::vector<VkFramebuffer> framebuffers;
    
public:
    ///
    void            initialize();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    Device(GPU &gpu, bool aa = false, std::vector<const char*> *validation = null);
    VkQueue &operator()(GPU::Capability cap);
};

