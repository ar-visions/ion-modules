#pragma once
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

#include <dx/dx.hpp>
#include <media/canvas.hpp>
#include <media/obj.hpp>

struct Device;
struct Texture;
///
VkDevice                handle(Device *device);
VkDevice         device_handle(Device *device);
VkPhysicalDevice    gpu_handle(Device *device);
uint32_t           memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props);
VkDevice             vk_device();
void         vk_subsystem_init();

#include <vk/window.hpp>
#include <vk/gpu.hpp>
#include <vk/device.hpp>
#include <vk/buffer.hpp>
#include <vk/texture.hpp>
#include <vk/descriptor.hpp>
#include <vk/render.hpp>
#include <vk/frame.hpp>
#include <vk/device.hpp>
#include <vk/vertex.hpp>
#include <vk/uniform.hpp>
#include <vk/pipeline.hpp>

struct Composer;
struct Vulkan {
    static void             init();
    static VkInstance       instance();
    static Device          &device();
    static VkPhysicalDevice gpu();
    static VkQueue          queue();
    static VkSurfaceKHR     surface(vec2i &);
    static VkImage          image();
    static uint32_t         queue_index();
    static uint32_t         version();
    static Canvas           canvas(vec2i);
    static void             set_title(str);
    static int              main(FnRender, Composer *);
    static Texture          texture(vec2i);
    static void             draw_frame();
    static recti            startup_rect();
};
