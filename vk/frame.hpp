#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/buffer.hpp>
#include <vk/uniform.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

struct MVP {
    alignas(16) m44f model;
    alignas(16) m44f view;
    alignas(16) glm::mat4 proj;
};

struct Frame {
    enum Attachment {
        Color,
        Depth,
        SwapView
    };
    int             index;
    Device         *device;
    array<Texture>    attachments;
    VkFramebuffer   framebuffer = VK_NULL_HANDLE;
    map<str, VkCommandBuffer> render_commands;
    void destroy();
    void update();
    operator VkFramebuffer &();
    Frame(Device *device);
};





