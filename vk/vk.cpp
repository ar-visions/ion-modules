#include <vk/window.hpp>
#include <vk/vk.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <dx/dx.hpp>
#include <ux/app.hpp>


//#include <glm/ext.hpp>

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

VkDevice vk_device() {
    return Vulkan::device();
}

const char *cstr_copy(const char *s) {
    int   len = strlen(s);
    char   *r = (char *)malloc(len + 1);
    std::memcpy((void *)r, (void *)s, len + 1);
    return (const char *)r;
}

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

struct Internal {
    Window *window;
    VkInstance vk = VK_NULL_HANDLE;
    vec<VkLayerProperties> layers;
    Device   device;
    GPU      gpu;
    Device   instance;
    Texture  tx_skia;
    bool     resize;
    vec<VkFence> fence;
    vec<VkFence> image_fence;
    Internal &bootstrap();
    static Internal &handle();
};

static Internal _;

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessageTypeFlagsEXT             type,
            const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
            void*                                       user_data) {
    std::cerr << "validation layer: " << cb_data->pMessage << std::endl;

    return VK_FALSE;
}

/// we wont even call it window manager-related nonsense. its a subsystem, not even a system. this will be a no-op some day.
void vk_subsystem_init() {
    static std::atomic<bool> init;
    if (!init) {
        init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
}

Internal &Internal::handle() { return _.vk ? _ : _.bootstrap(); }
Internal &Internal::bootstrap() {
    vk_subsystem_init();
    
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    layers.resize(count);
    vkEnumerateInstanceLayerProperties(&count, layers);
    ///
    VkApplicationInfo appInfo {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "ion";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "ion";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_0;
    ///
    VkInstanceCreateInfo ci {};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ///
    uint32_t    n_ext;
    auto          dbg = VkDebugUtilsMessengerCreateInfoEXT {};
    auto     glfw_ext = (const char **)glfwGetRequiredInstanceExtensions(&n_ext); /// this is ONLY for instance; must be moved.
    auto instance_ext = vec<const char *>(n_ext + 1);
    for (int i = 0; i < n_ext; i++)
        instance_ext += glfw_ext[i];
    instance_ext     += "VK_KHR_get_physical_device_properties2";
    #if !defined(NDEBUG)
        instance_ext          += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        ci.enabledLayerCount   = uint32_t(validation_layers.size());
        ci.ppEnabledLayerNames = (const char* const*)validation_layers.data();
        dbg.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg.messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg.messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg.pfnUserCallback    = vk_debug;
        ci.pNext               = (VkDebugUtilsMessengerCreateInfoEXT *) &dbg;
    #else
        ci.enabledLayerCount   = 0;
        ci.pNext               = nullptr;
    #endif
    ci.enabledExtensionCount   = static_cast<uint32_t>(instance_ext.size());
    ci.ppEnabledExtensionNames = instance_ext;
    auto res                   = vkCreateInstance(&ci, nullptr, &vk);
    assert(res == VK_SUCCESS);
    ///
    gpu    = GPU::select();
    device = Device(gpu, true);
    return *this;
}

void Vulkan::init() {
    Internal::handle();
}

/// fix the anti-pattern implementation here
Texture Vulkan::texture(vec2i size) {
    auto &i = Internal::handle();
    i.tx_skia.destroy();
    i.tx_skia = Texture { &i.device, size, rgba { 1.0, 0.0, 1.0, 1.0 },
        VK_IMAGE_USAGE_SAMPLED_BIT          | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT     | VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
        false, VK_FORMAT_R8G8B8A8_UNORM, -1 };
    i.tx_skia.push_stage(Texture::Stage::Color);
    return i.tx_skia;
}

static recti s_rect;
///
recti Vulkan::startup_rect() {
    assert(s_rect);
    return s_rect;
}

/// allocate composer, the class which takes rendered elements and manages instances
/// allocate window internal (glfw subsystem) and canvas (which initializes a Device for usage on skia?)
int Vulkan::main(Composer *composer) {
    Args     &args = composer->args;
    vec2i       sz = {args.count("window-width")  ? int(args["window-width"])  : 512,
                      args.count("window-height") ? int(args["window-height"]) : 512};
    s_rect         = recti { 0, 0, sz.x, sz.y };
    Internal    &i = Internal::handle();
    Window      &w = *i.window;
    Device &device =  i.device;
    Canvas  canvas = Canvas(sz, Canvas::Context2D);
    Texture &tx_canvas = i.tx_skia;
    ///
    composer->sz = &w.size;
    composer->ux->canvas = canvas;
    ///
    i.device.initialize(&w);
    w.show();
    /// canvas polygon data (pos, norm, uv, color)
    auto   vertices = Vertex::square();
    auto    indices = vec<uint16_t> { 0, 1, 2, 2, 3, 0 };
    auto        vbo = VertexBuffer<Vertex>(device, vertices);
    auto        ibo = IndexBuffer<uint16_t>(device, indices);
    auto        uni = UniformBuffer<MVP>(device, 0, [&](MVP &mvp) {
        /// consider this UniformData, it just creates a sub lambda in its utility template
        mvp         = {
             .model = glm::mat4(1.0f),
             .view  = glm::mat4(1.0f),
             .proj  = glm::ortho(-0.5, 0.5, 0.5, -0.5, 0.5, -0.5)
        };
    });
    auto         pl = Pipeline<Vertex> {
        device, uni, vbo, ibo, {
            Position3f(), Normal3f(), Texture2f(tx_canvas), Color4f()
        }, {0.05, 0.05, 0.2, 1.0}, std::string("main") /// canvas clear color is gray
    };
    
    /// prototypes add a Window&
    w.fn_cursor  = [&](double x, double y)         { composer->cursor(w, x, y);    };
    w.fn_mbutton = [&](int b, int a, int m)        { composer->button(w, b, a, m); };
    w.fn_resize  = [&]()                           { composer->resize(w);          };
    w.fn_key     = [&](int k, int s, int a, int m) { composer->key(w, k, s, a, m); };
    w.fn_char    = [&](uint32_t c)                 { composer->character(w, c);    };
    
    /// uniforms are meant to be managed by the app, passed into pipelines.
    w.loop([&]() {
        static bool init = false;
        if (!init) {
            vk_subsystem_init();
            init = true;
        }
        ///
        canvas.clear(rgba {0.0, 0.01, 0.05, 1.0});
        composer->render();
        canvas.flush();
        ///
        i.tx_skia.push_stage(Texture::Stage::Shader);
        device.render.push(pl);
        device.render.present();
        i.tx_skia.pop_stage();
        ///
        if (composer->root)
            w.set_title(composer->root->m.text.label);
        ///
        if (!composer->process())
            glfwWaitEventsTimeout(1.0);
    });
    return 0;
}

VkInstance Vulkan::instance() {
    return Internal::handle().vk;
}

Device &Vulkan::device() {
    return Internal::handle().device;
}

VkPhysicalDevice Vulkan::gpu() {
    return Internal::handle().device; /// make sure theres not a cross up during initialization
}

VkQueue Vulkan::queue() {
    return Internal::handle().device.queues[GPU::Graphics];
}

VkSurfaceKHR Vulkan::surface(vec2i &sz) {
    Internal &i = Internal::handle();
    if (!i.window) {
         i.window = new Window(sz);
         i.window->fn_resize = [i=i]() { ((Internal &)i).resize = true; };
    }
    return *i.window;
}

uint32_t Vulkan::queue_index() {
    return Internal::handle().device.gpu.index(GPU::Graphics);
}

uint32_t Vulkan::version() {
    return VK_API_VERSION_1_0;
}
