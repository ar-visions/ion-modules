#pragma once
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
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
