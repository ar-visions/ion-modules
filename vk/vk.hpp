#pragma once
#include <vulkan/vulkan.hpp>
#include <dx/dx.hpp>
#include <media/canvas.hpp>
#include <vk/gpu.hpp>
#include <vk/device.hpp>
#include <vk/buffer.hpp>
#include <vk/texture.hpp>
#include <vk/pipeline.hpp> // this one is tricky to organize because of Shader binding

struct Vulkan {
    static void             init();
    static VkInstance       instance();
    static Device          &device();
    static GPU             &gpu();
    static VkQueue          queue();
    static VkSurfaceKHR     surface();
    static VkSwapchainKHR   swapchain();
    static VkImage          image();
    static uint32_t         queue_index();
    static uint32_t         version();
    static Canvas           canvas(vec2i sz);
    static void             set_title(str s);
    static int              main(FnRender fn);
};
