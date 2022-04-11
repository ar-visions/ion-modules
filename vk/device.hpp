#pragma once
#include <vk/vk.hpp>
#include <vk/opaque.hpp>
#include <vk/render.hpp>
#include <vk/descriptor.hpp>
#include <vk/frame.hpp>
#include <vk/texture.hpp>
#include <vk/command.hpp>

/// vulkan people like to flush the entire glyph cache per type
typedef array<VkVertexInputAttributeDescription> VAttribs;

struct PipelineData;
struct Window;
struct Texture;
struct iDevice;
struct iTexture;

struct ResourceData {
    Texture *texture;
    Image   *image;
};

typedef map<Path, ResourceData> ResourceCache;
struct PipelineData;

struct Device {
    enum Module {
        Vertex,
        Fragment,
        Compute
    };
    ///
    sh<iDevice> intern;
    ///
    Device(nullptr_t n = null);
    Device(GPU &gpu, bool aa);
    ///
    static  Device &null_device();
    ///
    void                 update();
    Device &               sync();
    void             initialize(Window *window);
    void                command(std::function<void(VkCommandBuffer &)>);
    void                destroy();
    VkQueue              &queue(GPU::Capability cap);
    void                 module(Path, VAttribs &, Assets &, Module, VkShaderModule &);
    uint32_t        memory_type(uint32_t types, VkMemoryPropertyFlags props);
    ResourceCache    &get_cache();
    void                   push(PipelineData &pd);
    /// 
    Device  &         operator=(const Device &dev);
    ///
    operator VkPhysicalDevice &();
    operator VkCommandPool    &();
    operator VkViewport       &();
    operator VkDevice         &();
    operator VkRenderPass     &();
    operator VkPipelineViewportStateCreateInfo &();
    operator VkPipelineRasterizationStateCreateInfo &();
};
