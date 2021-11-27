#include <vk/window.hpp>
#include <vk/vk.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <dx/dx.hpp>
#include <ux/app.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>

#include <gpu/gl/GrGLInterface.h>

const char *cstr_copy(const char *s) {
    int   len = strlen(s);
    char   *r = (char *)malloc(len + 1);
    std::memcpy((void *)r, (void *)s, len + 1);
    return (const char *)r;
}

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

struct Internal {
    VkInstance vk;
    Device   device;
    Window  &window;
    Canvas   canvas;
    vec<GPU> gpus;
    Device   instance;
    Texture  tx;
    bool     resize;
    vec<VkFence> fence;
    vec<VkFence> image_fence;
    vec<VkLayerProperties> layers;
    Internal(Window &w);
};

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessageTypeFlagsEXT             type,
            const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
            void*                                       user_data) {
    std::cerr << "validation layer: " << cb_data->pMessage << std::endl;

    return VK_FALSE;
}

static inline Internal &intern() {
    static Internal *i;
    if (i) return *i;
    auto *w = new Window(vec2i { 1, 1 });
    i       = new Internal { *w };
    ///
    w->fn_resize = [i=i]() { i->resize = true; };
    return *i;
}

Internal::Internal(Window &w) : window(w) {
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    layers.resize(count);
    vkEnumerateInstanceLayerProperties(&count, layers);
    
    VkApplicationInfo appInfo {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "ion";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "ion";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_0;

    VkInstanceCreateInfo ci {};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    
    uint32_t    n_ext;
    auto          dbg = VkDebugUtilsMessengerCreateInfoEXT {};
    auto     glfw_ext = (const char **)glfwGetRequiredInstanceExtensions(&n_ext); /// this is ONLY for instance; must be moved.
    auto instance_ext = vec<const char *>(n_ext);
    for (int i = 0; i < n_ext; i++)
        instance_ext += glfw_ext[i];
    ///
    ci.enabledExtensionCount   = static_cast<uint32_t>(instance_ext.size());
    ci.ppEnabledExtensionNames = instance_ext;
    ///
    #ifndef NDEBUG
        instance_ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        ci.enabledLayerCount = uint32_t(validation_layers.size());
        ci.ppEnabledLayerNames = (const char* const*)validation_layers.data();
        ///
        dbg.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg.pfnUserCallback = vk_debug;
        ///
        ci.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &dbg;
    #else
        ci.enabledLayerCount = 0;
        ci.pNext = nullptr;
    #endif
    ///
    assert(vkCreateInstance(&ci, nullptr, &vk) == VK_SUCCESS);
    
    
    device = Device();

}

void Vulkan::init() {
    intern();
}

Texture Vulkan::texture(vec2i size) {
    auto i = intern();
    i.tx.destroy();
    i.tx = Texture { &i.device, size, rgba { 1.0, 0.0, 0.0, 1.0 }, /// lets hope we see red soon.
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
        i.device.msaa_samples, VK_FORMAT_R8G8B8A8_SRGB, -1 };
    return i.tx;
}

/// an in-flight fence. makes sense
void Vulkan::draw_frame() {
}


///
int Vulkan::main(FnRender fn, Composer *composer) {
    Internal    &i = intern();
    Window      &w = i.window;
    Canvas &canvas = i.canvas;
    /// allocate composer, the class which takes rendered elements and manages instances
    
    /// allocate window internal (glfw subsystem) and canvas (which initializes a Device for usage on skia?)
    canvas = Canvas({int(w.size.x), int(w.size.y)}, Canvas::Context2D);
    Args args;
    
    w.loop([&]() {
        rectd box   = {   0,   0, 320, 240 };
        rgba  color = { 255, 255,   0, 255 };

        i.tx.transition_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
        canvas.color(color);
        canvas.clear(); /// todo: like the idea of fill() with a color just being a clear color.  thats a simpler api. the path is a param that can be null
        //canvas.fill(box);
        canvas.flush();
        i.tx.transition_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        draw_frame();

        Composer &cmp = *composer;
        cmp(fn(args));
        w.set_title(cmp.root->props.text.label);
        if (!cmp.process())
            glfwWaitEventsTimeout(1.0);
        
        //Vulkan::swap(sz);
        // glfw might just manage swapping? but lets remove it for now
        //glfwSwapBuffers(intern->glfw);
    });
    return 0;
}

VkInstance Vulkan::instance() {
    return intern().vk;
}

Device &Vulkan::device() {
    return intern().device;
}

VkPhysicalDevice Vulkan::gpu() {
    return intern().device; /// make sure theres not a cross up during initialization
}

VkQueue Vulkan::queue() {
    return intern().device.queues[GPU::Graphics];
}

VkSurfaceKHR Vulkan::surface() {
    return intern().device.gpu.surface;
}

uint32_t Vulkan::queue_index() {
    return intern().device.gpu.index(GPU::Graphics);
}

uint32_t Vulkan::version() {
    return VK_API_VERSION_1_0;
}
