#pragma once
#include <vk/vk.hpp>
#include <vk/render.hpp>
#include <vk/descriptor.hpp>
#include <vk/frame.hpp>
#include <vk/texture.hpp>

struct PipelineData;
struct Window;
struct Device {
    enum Module {
        Vertex,
        Fragment,
        Compute
    };
    void                  create_swapchain();
    void                  create_command_buffers();
    void                  create_render_pass();
    uint32_t              attachment_index(Texture *tx);
    map<str, VkShaderModule> f_modules;
    map<str, VkShaderModule> v_modules;
    Render                render;
    VkSampleCountFlagBits sampling    = VK_SAMPLE_COUNT_8_BIT;
    VkRenderPass          render_pass = VK_NULL_HANDLE;
    Descriptor            desc;
    GPU                   gpu;
    VkCommandPool         pool;
    VkDevice              device;
    VkQueue               queues[GPU::Complete];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    vec<Frame>            frames;
    vec<VkImage>          swap_images;
    Texture               tx_color;
    Texture               tx_depth;
    
    static Device &null_device();
    
    VkShaderModule module(std::filesystem::path p, Module type);
    PipelineData &operator[](std::string n);
    ///
    void            initialize(Window *);
    /// todo: initialize for compute_only
    void            update();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    uint32_t        memory_type(uint32_t types, VkMemoryPropertyFlags props);
    
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    operator        VkRenderPass();
    Device(GPU &gpu, bool aa = false);
    VkQueue &operator()(GPU::Capability cap);
    ///
    Device(nullptr_t n = null) { }
    operator VkViewport &() { return viewport; }
    operator VkPipelineViewportStateCreateInfo() {
        sc.offset = {0, 0};
        sc.extent = extent;
        return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, null, 0, 1, &viewport, 1, &sc };
    }
    operator VkPipelineRasterizationStateCreateInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, null, 0, VK_FALSE, VK_FALSE,
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE // ... = 0.0f
        };
    }
};
