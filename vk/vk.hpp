#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <data/data.hpp>

struct Vulkan {
    static int run(std::function<void(void)> fn);
    
    static GLFWwindow      *get_window();
    static VkInstance       get_instance();
    static VkDevice         get_device();
    static VkPhysicalDevice get_gpu();
    static VkQueue          get_queue();
    static VkSurfaceKHR     get_surface();
    static VkSwapchainKHR   get_swapchain();
    static VkImage          get_image();
    static uint32_t         get_queue_index();
    static uint32_t         get_version();
};
