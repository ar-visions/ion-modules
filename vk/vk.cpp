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

Internal &Internal::handle() { return _.vk ? _ : _.bootstrap(); }
Internal &Internal::bootstrap() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
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
    auto instance_ext = vec<const char *>(n_ext + 1);
    for (int i = 0; i < n_ext; i++)
        instance_ext += glfw_ext[i];
    instance_ext += "VK_KHR_get_physical_device_properties2";
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
    
    gpu    = GPU::select();
    device = Device(gpu, true);
    return *this;
}

void Vulkan::init() {
    Internal::handle();
}

Texture Vulkan::texture(vec2i size) {
    auto &i = Internal::handle();
    //i.tx_skia.destroy();
    // lets call this something descriptive
    
    // VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    i.tx_skia = Texture { &i.device, size, rgba { 1.0, 0.0, 0.0, 1.0 }, /// lets hope we see red soon.
        VK_IMAGE_USAGE_SAMPLED_BIT|
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT|
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, // aspect? what is this on vk-aa
        false, VK_FORMAT_R8G8B8A8_UNORM, -1 }; // force sample count to 1 bit
    
    return i.tx_skia;
}

static recti s_rect;

recti Vulkan::startup_rect() {
    assert(s_rect);
    return s_rect;
}

///
/// allocate composer, the class which takes rendered elements and manages instances
/// allocate window internal (glfw subsystem) and canvas (which initializes a Device for usage on skia?)
int Vulkan::main(FnRender fn, Composer *composer) {
    Args     &args = composer->args;
    vec2i       sz = {args.count("window-width")  ? int(args["window-width"])  : 512,
                      args.count("window-height") ? int(args["window-height"]) : 512};
    s_rect         = recti { 0, 0, sz.x, sz.y };
    Internal    &i = Internal::handle();
    Window      &w = *i.window;
    Device &device = i.device;
    Canvas  canvas = Canvas(sz, Canvas::Context2D);
    Texture &tx_canvas = i.tx_skia;
    ///
    i.device.initialize(&w);
    w.show();
    /// canvas polygon data (pos, norm, uv, color)
    auto   vertices = Vertex::square();
    auto    indices = vec<uint16_t> { 0, 1, 2, 2, 3, 0 };
    auto        vbo = VertexBuffer<Vertex>(device, vertices);
    auto        ibo = IndexBuffer<uint16_t>(device, indices);
    auto        uni = UniformBuffer<MVP>(device, 0, [&](MVP &mvp) {
        mvp         = {
             .model = glm::mat4(1.0f),
             .view  = glm::mat4(1.0f),
             .proj  = glm::mat4(1.0f)
        };
    });
    auto         pl = Pipeline<Vertex> {
        device, uni, vbo, ibo, {
            Position3f(), Normal3f(), Texture2f(tx_canvas), Color4f()
        }, std::string("main")
    };
    
    /// uniforms are meant to be managed by the app, passed into pipelines.
    w.loop([&]() {
        static bool init = false;
        if (!init) {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            init = true;
        }
        rectd box   = {   0,   0, 320, 240 };
        rgba  color = { 255, 255,   0, 255 };
        //i.tx_skia.set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        canvas.color(color);
        canvas.clear();            /// todo: like the idea of fill() with a color just being a clear color.
        /*canvas.fill(box);*/      /// thats a simpler api. the path is a param that can be null
        canvas.flush();            ///
        /// i.tx_skia.set_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        i.tx_skia.set_stage(Texture::Stage::Shader);
        
        device.tx_color.set_stage(Texture::Stage::Color);
        device.tx_depth.set_stage(Texture::Stage::Depth);
        
        /// this needs to happen here...
        device.render.push(pl);
        device.render.present(); /// execute all graphic (render) pipelines [ on the current frame ], and present
        
        i.tx_skia.set_stage(Texture::Stage::Color);
        int test = 0;
        test++;
        //Composer &cmp = *composer;
        //cmp(fn(args));
        //w.set_title(cmp.root->props.text.label);
        
        //if (!cmp.process())
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
