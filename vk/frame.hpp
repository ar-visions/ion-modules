#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/buffer.hpp>
#include <vk/uniform.hpp>

struct MVP {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

/// What do you call this controller?
struct Frame {
    enum Attachment {
        Color,
        Depth,
        SwapView
    };
    int             index;
    Device         *device;
    vec<Texture>    attachments; // Textures can be references to others created elsewhere, with views of their own instancing
    VkFramebuffer   framebuffer = VK_NULL_HANDLE;
    map<str, VkCommandBuffer> render_commands;
    void destroy();
    void update();
    operator VkFramebuffer &();
    Frame(Device *device);
};





