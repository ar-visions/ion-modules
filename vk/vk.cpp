
#include <dx/dx.hpp>
#include <dx/async.hpp>

/// large include.
#include <vulkan/vulkan.hpp>

#include <vk/vk.hpp>
#include <vk/window.hpp> /// window is internal to vk atm. there was a space creature outside similiar to the one that ate the millenium falcon and i really dont want to go back

#include <media/canvas.hpp>

#include <vk/vertex.hpp>
#include <vk/opaque.hpp>
#include <vk/window.hpp>
#include <vk/device.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <dx/dx.hpp>
#include <ux/app.hpp>

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

cchar_t *cstr_copy(cchar_t *s) {
    int   len = strlen(s);
    char   *r = (char *)malloc(len + 1);
    std::memcpy((void *)r, (void *)s, len + 1);
    return (cchar_t *)r;
}

const std::vector<cchar_t*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

struct Internal {
    Window        *win;
    Device         dev;
    GPU            gpu;
    Texture        tx_skia;
    bool           resize;
    array<VkFence> fence;
    array<VkFence> image_fence;
    VkInstance     vk = VK_NULL_HANDLE;
    ///
    array<VkLayerProperties> layers;
    Internal        &bootstrap();
    static Internal &handle();
};

static Internal _;


static void glfw_error  (int code, cchar_t *cstr) {
    console.log("glfw error: {0}", {str(cstr)});
}

Window &Window::ref(GLFWwindow *h) {
    auto w = (Window *)glfwGetWindowUserPointer(h);
    return *w;
}

static void glfw_key    (GLFWwindow *h, int key, int scancode, int action, int mods) {
    auto &win = Window::ref(h);
    if (win.fn_key)
        win.fn_key(key, scancode, action, mods);
}

static void glfw_char   (GLFWwindow *h, uint32_t code) {
    auto &win = Window::ref(h);
    if (win.fn_char)
        win.fn_char(code);
}

static void glfw_mbutton(GLFWwindow *h, int button, int action, int mods) {
    auto &win = Window::ref(h);
    if (win.fn_mbutton)
        win.fn_mbutton(button, action, mods);
}

static void glfw_cursor (GLFWwindow *handle, double x, double y) {
    auto &win = Window::ref(handle);
    if (win.fn_cursor)
        win.fn_cursor(x, y);
}

static int resize_count = 0;

static void glfw_resize (GLFWwindow *handle, int32_t w, int32_t h) {
    auto &win = Window::ref(handle);
    win.size  = vec2i { w, h };
    if (win.fn_resize)
        win.fn_resize();
    resize_count++; /// test.
}

vec2 Window::cursor() {
    vec2 result = { 0, 0 };
    glfwGetCursorPos(handle, &result.x, &result.y);
    return result;
}

Window::operator GLFWwindow *() {
    return handle;
}

str Window::clipboard() {
    return glfwGetClipboardString(handle);
}

void Window::set_clipboard(str text) {
    glfwSetClipboardString(handle, text.cstr());
}

Window::operator VkSurfaceKHR &() {
    if (!surface) {
        auto   vk = Vulkan::instance();
        auto code = glfwCreateWindowSurface(vk, handle, null, &surface);
        assert(code == VK_SUCCESS);
    }
    return surface;
}

void Window::set_title(str s) {
    title = s;
    glfwSetWindowTitle(handle, title.cstr());
}

void Window::show() {
    glfwShowWindow(handle);
}

void Window::hide() {
    glfwHideWindow(handle);
}

Window *Window::first = null;

/// Window Constructors
Window::Window(std::nullptr_t n) { }
Window::Window(vec2i     sz): size(sz) {
    handle = glfwCreateWindow(sz.x, sz.y, title.cstr(), nullptr, nullptr);
    ///
    glfwSetWindowUserPointer      (handle, this);
    glfwSetFramebufferSizeCallback(handle, glfw_resize);
    glfwSetKeyCallback            (handle, glfw_key);
    glfwSetCursorPosCallback      (handle, glfw_cursor);
    glfwSetCharCallback           (handle, glfw_char);
    glfwSetMouseButtonCallback    (handle, glfw_mbutton);
    glfwSetErrorCallback          (glfw_error);
}

GLFWAPI int glfwIsInit(void);

int Window::loop(std::function<void()> fn) {
    /// render loop with node event processing
    while (!glfwWindowShouldClose(handle)) {
        fn();
        glfwPollEvents();
    }
    return 0;
}

///
/// Window Destructor
Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

struct vkState {
    VkPipelineVertexInputStateCreateInfo    vertex_info {};
    VkPipelineInputAssemblyStateCreateInfo  topology    {};
    VkPipelineViewportStateCreateInfo       vs          {};
    VkPipelineRasterizationStateCreateInfo  rs          {};
    VkPipelineMultisampleStateCreateInfo    ms          {};
    VkPipelineDepthStencilStateCreateInfo   ds          {};
    VkPipelineColorBlendAttachmentState     cba         {};
    VkPipelineColorBlendStateCreateInfo     blending    {};
};
        
typedef array<VkQueueFamilyProperties> FamilyProps;

Descriptor::Descriptor(Device *dev) : dev(dev) { }
Descriptor &Descriptor::operator=(const Descriptor &d) {
    if (this != &d) {
        dev   = d.dev;
        pool  = (sh<VkDescriptorPool> &)d.pool;
    }
    return *this;
}

void Descriptor::destroy() {
    VkDevice &d = *dev;
    vkDestroyDescriptorPool(d, pool, nullptr);
}

struct Device;
struct iGPU {
protected:
    var gfx;
    var present;
public:
    VkSurfaceCapabilitiesKHR   caps;
    array<VkSurfaceFormatKHR>  formats;
    array<VkPresentModeKHR>    present_modes;
    bool                       ansio;
    VkSampleCountFlagBits      multi_sample;
    VkPhysicalDevice           gpu = VK_NULL_HANDLE;
    FamilyProps                family_props;
    VkSurfaceKHR               surface;
    VkQueue                    queues[2];
    
    VkSampleCountFlagBits update_sampling() {
        if (ansio) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(gpu, &props);
            VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                        props.limits.framebufferDepthSampleCounts;
            ///
            if (counts & VK_SAMPLE_COUNT_64_BIT) { multi_sample = VK_SAMPLE_COUNT_64_BIT; } else
            if (counts & VK_SAMPLE_COUNT_32_BIT) { multi_sample = VK_SAMPLE_COUNT_32_BIT; } else
            if (counts & VK_SAMPLE_COUNT_16_BIT) { multi_sample = VK_SAMPLE_COUNT_16_BIT; } else
            if (counts & VK_SAMPLE_COUNT_8_BIT)  { multi_sample = VK_SAMPLE_COUNT_8_BIT;  } else
            if (counts & VK_SAMPLE_COUNT_4_BIT)  { multi_sample = VK_SAMPLE_COUNT_4_BIT;  } else
            if (counts & VK_SAMPLE_COUNT_2_BIT)  { multi_sample = VK_SAMPLE_COUNT_2_BIT;  }
                else multi_sample = VK_SAMPLE_COUNT_1_BIT;
        } else
            multi_sample = VK_SAMPLE_COUNT_1_BIT;
        return multi_sample;
    }
    
    iGPU(nullptr_t = null) { }
    
    /// internal GPU constructor
    iGPU(VkPhysicalDevice &gpu, VkSurfaceKHR &surface)
    {
        uint32_t fcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, nullptr);
        family_props.resize(fcount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, family_props.data());
        uint32_t index = 0;

        /// we need to know the indices of these
        gfx     = var(false);
        present = var(false);
        
        for (const auto &fprop: family_props) { // check to see if it steps through data here.. thats the question
            VkBool32 has_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &has_present);
            bool has_gfx = (fprop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            ///
            /// prefer to get gfx and present in the same family_props
            if (has_gfx) {
                gfx = var(index);
                /// query surface formats
                uint32_t format_count;
                vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
                if (format_count != 0) {
                    formats.resize(format_count);
                    vkGetPhysicalDeviceSurfaceFormatsKHR(
                        gpu, surface, &format_count, formats.data());
                }
                /// query swap chain support, store on GPU
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    gpu, surface, &caps);
                /// ansio relates to gfx
                VkPhysicalDeviceFeatures fx;
                vkGetPhysicalDeviceFeatures(gpu, &fx);
                ansio = fx.samplerAnisotropy;
            }
            ///
            if (has_present) {
                present = var(index);
                uint32_t present_mode_count;
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                    gpu, surface, &present_mode_count, nullptr);
                if (present_mode_count != 0) {
                    present_modes.resize(present_mode_count);
                    vkGetPhysicalDeviceSurfacePresentModesKHR(
                        gpu, surface, &present_mode_count, present_modes.data());
                }
            }
            if (has_gfx & has_present)
                break;
            index++;
        }
        update_sampling();
    }

    bool has_capability(GPU::Capability caps) {
        bool g = gfx     == Type::ui32;
        bool p = present == Type::ui32;
        if (caps == GPU::Capability::Complete)
            return g && p;
        if (caps == GPU::Capability::Graphics)
            return g;
        if (caps == GPU::Capability::Present)
            return p;
        return true;
    }

    bool supported() {
        return has_capability(GPU::Complete) && formats && present_modes && ansio;
    }
    
    uint32_t index(GPU::Capability caps) {
        assert(gfx == Type::ui32 || present == Type::ui32);
        if (caps == GPU::Graphics && has_capability(GPU::Graphics))
            return uint32_t(gfx);
        if (caps == GPU::Present  && has_capability(GPU::Present))
            return uint32_t(present);
        assert(false);
        return 0;
    }
};

struct Device;
/// internal Device, protect precious, all of the apps that should compile faster
struct iDevice {
    Device *              dev;
    map<str, VkShaderModule> f_modules;
    map<str, VkShaderModule> v_modules;
    Render                render;
    VkSampleCountFlagBits sampling    = VK_SAMPLE_COUNT_8_BIT;
    VkRenderPass          render_pass = VK_NULL_HANDLE;
    Descriptor            desc;
    GPU                   gpu;
    VkCommandPool         pool;
    VkDevice              device;
    VkQueue               queues[2];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    array<Frame>          frames;
    array<VkImage>        swap_images; // force re-render on watch event.
    Texture               tx_color;
    Texture               tx_depth;
    map<Path, ResourceData> resource_cache;
    
    iDevice(nullptr_t n = null) { }
    iDevice(Device *dev, GPU &gpu, bool aa) : dev(dev), sampling(aa ? VK_SAMPLE_COUNT_8_BIT : VK_SAMPLE_COUNT_1_BIT), gpu(gpu) { }

    void     create_render_pass() {
        Frame &f0                     = frames[0];
        Texture &tx_ref               = f0.attachments[Frame::SwapView]; /// has msaa set to 1bit
        assert(f0.attachments.size() == 3);
        VkAttachmentReference   &cref = tx_color; // VK_IMAGE_LAYOUT_UNDEFINED
        VkAttachmentReference   &dref = tx_depth; // VK_IMAGE_LAYOUT_UNDEFINED
        VkAttachmentReference   &rref = tx_ref;   // the odd thing is the associated COLOR_ATTACHMENT layout here;
        VkSubpassDescription     sp   = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, null, 1, &cref, &rref, &dref };

        VkSubpassDependency dep { };
        dep.srcSubpass                = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass                = 0;
        dep.srcStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask             = 0;
        dep.dstStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask             = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription adesc_clr_image  = f0.attachments[0];
        VkAttachmentDescription adesc_dep_image  = f0.attachments[1];
        VkAttachmentDescription adesc_swap_image = f0.attachments[2];
        
        std::array<VkAttachmentDescription, 3> attachments = {adesc_clr_image, adesc_dep_image, adesc_swap_image};
        VkRenderPassCreateInfo rpi { }; // VkAttachmentDescription needs to be 4, 4, 1
        rpi.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpi.attachmentCount           = uint32_t(attachments.size());
        rpi.pAttachments              = attachments.data();
        rpi.subpassCount              = 1;
        rpi.pSubpasses                = &sp;
        rpi.dependencyCount           = 1;
        rpi.pDependencies             = &dep;
        assert(vkCreateRenderPass(device, &rpi, nullptr, &render_pass) == VK_SUCCESS);
    }
    
    void     module(Path path, VAttribs &vattr, Assets &assets, Device::Module type, VkShaderModule &module) {
        str      src_path = path;
        auto           &m = type == Device::Module::Vertex ? v_modules : f_modules;
        str          defs = "";
        uint32_t    flags = 0;
        static auto uname = map<Asset::Type, str> {
            { Asset::Color,    "COLOR"    },
            { Asset::Shine,    "SHINE"    },
            { Asset::Rough,    "ROUGH"    },
            { Asset::Displace, "DISPLACE" },
            { Asset::Normal,   "NORMAL"   }
        };
        
        /// define used attribute location
        for (auto &a: vattr)
            defs += fmt {" -DLOCATION_{0}={0}", {a.location}}; //can use it within layout() per attrib in glsl, so if its not defined the shader breaks, thats an important feedback there.
        
        for (auto &[type, tx]: assets) { /// always use type when you can, over id (id is genuinely instanced things)
            defs  += fmt { " -D{0}={1}", { uname[type], uint32_t(type) }};
            flags |= uint32_t(1 << type);
        }
        Path   spv = fmt {"{0}.{1}.spv", { src_path, str(int(flags)) }};
        
    #if !defined(NDEBUG)
            if (!spv.exists() || path.modified_at() > spv.modified_at()) {
                if (m.count(src_path))
                    m.erase(src_path);
                
                /// call glslc compiler with definitions, input file is the .vert or .frag passed in, and output is our keyed use-case shader
                Path        tmp = "./.tmp/"; ///
                str     command = fmt {"/usr/local/bin/glslc{0} {1} -o {2}", { defs, src_path, spv }};
                async { command }.sync();
                bool     exists = spv.exists();
                
                /// if it succeeds, the spv is written and that will have a greater modified-at
                if (exists && spv.modified_at() >= path.modified_at())
                    spv.copy(tmp);
                else if (!exists) {
                    const bool use_previous = true;
                    if (use_previous) {
                        /// try the old temp (previously succeeded build, if avail)
                        Path prev_spv = tmp / (spv.stem() + ".spv");
                        if (!prev_spv.exists())
                            console.fault("failure to find backup of shader: {0}", { path.stem() });
                        spv = prev_spv;
                    } else
                        console.fault("shader compilation failed: {0}", { path.stem() });
                }
            }
    #endif

        if (!m.count(src_path)) {
            auto mc     = VkShaderModuleCreateInfo { };
            str code    = str::read_file(spv);
            mc.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            mc.codeSize = code.size();
            mc.pCode    = reinterpret_cast<const uint32_t *>(code.cstr());
            assert (vkCreateShaderModule(device, &mc, null, &m[src_path]) == VK_SUCCESS);
        }
        module = m[src_path];
    }
    
    void initialize(Window *win) {
        auto select_surface_format = [](array<VkSurfaceFormatKHR> &formats) -> VkSurfaceFormatKHR {
            for (const auto &f: formats)
                if (f.format     == VK_FORMAT_B8G8R8A8_UNORM && // VK_FORMAT_B8G8R8A8_UNORM in an attempt to fix color issues, no effect.
                    f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // the only colorSpace available in list of formats
                    return f;
            return formats[0];
        };
        ///
        auto select_present_mode = [](array<VkPresentModeKHR> &modes) -> VkPresentModeKHR {
            for (const auto &m: modes)
                if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                    return m;
            return VK_PRESENT_MODE_FIFO_KHR;
        };
        ///
        auto swap_extent = [](VkSurfaceCapabilitiesKHR& caps) -> VkExtent2D {
            if (caps.currentExtent.width != UINT32_MAX) {
                return caps.currentExtent;
            } else {
                int w, h;
                glfwGetFramebufferSize(*Window::first, &w, &h);
                VkExtent2D ext = { uint32_t(w), uint32_t(h) };
                ext.width      = std::clamp(ext.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
                ext.height     = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
                return ext;
            }
        };
        ///
        iGPU   & igpu                 =  gpu.intern;
        uint32_t gfx_index            =  gpu.index(GPU::Graphics);
        uint32_t present_index        =  gpu.index(GPU::Present);
        auto     surface_format       =  select_surface_format(igpu.formats); /// messy a bit
        auto     surface_present      =  select_present_mode  (igpu.present_modes);
        auto     extent               =  swap_extent          (igpu.caps);
        uint32_t frame_count          =  igpu.caps.minImageCount + 1;
        uint32_t indices[]            =  { gfx_index, present_index };
        ///
        if (igpu.caps.maxImageCount > 0 && frame_count > igpu.caps.maxImageCount)
            frame_count               = igpu.caps.maxImageCount;
        ///
        VkSwapchainCreateInfoKHR ci {};
        ci.sType                      =  VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface                    = *win;
        ci.minImageCount              =  frame_count;
        ci.imageFormat                =  surface_format.format;
        ci.imageColorSpace            =  surface_format.colorSpace;
        ci.imageExtent                =  extent;
        ci.imageArrayLayers           =  1;
        ci.imageUsage                 =  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ///
        if (gfx_index != present_index) {
            ci.imageSharingMode       =  VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount  =  2;
            ci.pQueueFamilyIndices    =  indices;
        } else
            ci.imageSharingMode       =  VK_SHARING_MODE_EXCLUSIVE;
        ///
        ci.preTransform               =  igpu.caps.currentTransform;
        ci.compositeAlpha             =  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode                =  surface_present;
        ci.clipped                    =  VK_TRUE;
        
        /// create swap-chain
        assert(vkCreateSwapchainKHR(device, &ci, nullptr, &swap_chain) == VK_SUCCESS);
        vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, nullptr); /// the dreaded frame_count (2) vs image_count (3) is real.
        frames = array<Frame>( Size(frame_count), [&](size_t i) -> Frame { return Frame((Device *)this); } );
        
        /// get swap-chain images
        swap_images.resize(frame_count);
        vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
        format             = surface_format.format;
        this->extent       = extent;
        viewport           = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
        
        /// create descriptor pool (to my mind i have no idea why this is here.  its repeated in its entirety for hte pipeline)
        /// it seems completely 1:1 relationship with your maximum resource user, although i could be wrong
        const int guess    = 8;
        auto      ps       = std::array<VkDescriptorPoolSize, 7> {};
        ps[0]              = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         frame_count * guess };
        
        for (int i = 1; i < 7; i++)
            ps[i]          = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count * guess };
        
        auto dpi           = VkDescriptorPoolCreateInfo {
                                VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                                frame_count * guess, uint32_t(ps.size()), ps.data() };
        render             = Render(dev);
        desc               = Descriptor(dev);
        ///
        assert(vkCreateDescriptorPool(device, &dpi, nullptr, (VkDescriptorPool *)desc.pool) == VK_SUCCESS);
        for (auto &f: frames)
            f.index        = -1;
        update();
    }

    /// todo: initialize for compute_only
    void update() {
        auto     sz          = vec2i { int(extent.width), int(extent.height) };
        uint32_t frame_count = frames.size();
        render.updating();
        tx_color = ColorTexture { dev, sz, null };
        tx_depth = DepthTexture { dev, sz, null };
        /// should effectively perform: ( Undefined -> Transfer, Transfer -> Shader, Shader -> Color )
        for (size_t i = 0; i < frame_count; i++) {
            Frame &frame   = frames[i];
            frame.index    = i;
            auto   tx_swap = SwapImage { dev, sz, swap_images[i] };
            ///
            frame.attachments = array<Texture> {
                tx_color,
                tx_depth,
                tx_swap
            };
        }
        ///
        create_render_pass();
        tx_color.push_stage(Texture::Stage::Shader);
        tx_depth.push_stage(Texture::Stage::Shader);
        ///
        for (size_t i = 0; i < frame_count; i++) {
            Frame &frame = frames[i];
            frame.update();
        }
        tx_color.pop_stage();
        tx_depth.pop_stage();
        /// render needs to update
        /// render.update();
    }

    void destroy() {
        for (auto &f: frames)
            f.destroy();
        for (auto &[k,f]: f_modules)
            vkDestroyShaderModule(device, f, nullptr);
        for (auto &[k,v]: v_modules)
            vkDestroyShaderModule(device, v, nullptr);
        ///
        vkDestroyRenderPass(device, render_pass, nullptr);
        vkDestroySwapchainKHR(device, swap_chain, nullptr);
        desc.destroy();
    }
    
    ///
    void command(std::function<void(VkCommandBuffer &)> fn) {
        VkCommandBuffer cb;
        VkCommandPool   pl = (VkCommandPool &)pool;
        VkCommandBufferAllocateInfo ai {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool        = pl,
            .commandBufferCount = 1
        };
        vkAllocateCommandBuffers(device, &ai, &cb);

        VkCommandBufferBeginInfo bi {};
        bi.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);

        /// run command
        fn(cb);

        vkEndCommandBuffer(cb);
        ///
        VkSubmitInfo si {};
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &cb;
        ///
        VkQueue &queue          = queues[GPU::Graphics];
        vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue); /// should be based on flags
        ///
        vkFreeCommandBuffers(device, pl, 1, &cb);
    }

    uint32_t memory_type(uint32_t types, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties mprops;
        vkGetPhysicalDeviceMemoryProperties(gpu, &mprops);
        ///
        for (uint32_t i = 0; i < mprops.memoryTypeCount; i++)
            if ((types & (1 << i)) && (mprops.memoryTypes[i].propertyFlags & props) == props)
                return i;
        /// no flags match
        assert(false);
        return 0;
    }

    void sync() {
        usleep(1000); // todo: check the de-fencing things.
    }

    operator VkPhysicalDevice &() { return (VkPhysicalDevice &)gpu;         }
    operator VkCommandPool    &() { return (VkCommandPool    &)pool;        }
    operator VkDevice         &() { return (VkDevice         &)device;      }
    operator VkRenderPass     &() { return (VkRenderPass     &)render_pass; }

    operator VkPipelineViewportStateCreateInfo &() {
        sc.offset = { 0, 0 };
        sc.extent = extent;
        static VkPipelineViewportStateCreateInfo info {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, null, 0, 1, &viewport, 1, &sc };
        return info;
    }

    operator VkPipelineRasterizationStateCreateInfo &() {
        static VkPipelineRasterizationStateCreateInfo info {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            null, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE
        };
        return info;
    }
};

struct iImagePair {
    Texture *texture;
    Image   *image;
};


struct iPipelineData {
    Device                    *dev;
    std::string                shader;          /// name of the shader.  worth saying.
    array<VkCommandBuffer>     frame_commands;  /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
    array<VkDescriptorSet>     desc_sets;       /// needs to be in frame, i think.
    VkPipelineLayout           pipeline_layout; /// and what a layout
    VkPipeline                 pipeline;        /// pipeline handle
    UniformData                ubo;             /// we must broadcast this buffer across to all of the swap uniforms
    VertexData                 vbo;             ///
    IndexData                  ibo;
    // array<VkVertexInputAttributeDescription> attr; -- vector is given via VertexData::fn_attribs(). i dont believe those need to be pushed up the call chain
    Assets                     assets;
    VkDescriptorSetLayout      set_layout;
    bool                       enabled = true;
    size_t                     vsize;
    int                        sync = -1;

    iPipelineData(std::nullptr_t    n = null) { }

    operator bool  ()                 { return  dev &&  enabled; }
    bool operator! ()                 { return !dev || !enabled; }
    void   enable  (bool en)          { enabled = en; }
    void update    (size_t frame_id)  { /* todo: no-op right now */ }
    void destroy() {
        Device  &dev = *this->dev;
        VkDevice  &d = dev;
        vkDestroyDescriptorSetLayout(d, set_layout, null);
        vkDestroyPipeline(d, pipeline, null);
        vkDestroyPipelineLayout(d, pipeline_layout, null);
        VkCommandPool &pool = dev;
        for (auto &cmd: frame_commands)
            vkFreeCommandBuffers(d, pool, 1, &cmd);
    }

    ~iPipelineData() {
        destroy();
    }

    /// constructor for pipeline memory; worth saying again.
    iPipelineData(Device        &dev,     UniformData &ubo,
                  VertexData    &vbo,     IndexData   &ibo,
                  Assets     &assets,     size_t       vsize, // replace all instances of array<Texture *> with a map<str, Teture *>
                  string       shader,    VkStateFn    vk_state):
               dev(&dev),  shader(shader),   ubo(ubo),
               vbo(vbo),      ibo(ibo),   assets(assets),  vsize(vsize) {
        ///
        auto bindings  = array<VkDescriptorSetLayoutBinding>(1 + assets.size());
             bindings += { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
        
        /// binding ids for resources are reflected in shader
        /// todo: write big test for this
        for (auto &[r, tx]:assets) {
            VkDescriptorSetLayoutBinding desc;
            Asset::descriptor(r, desc);
            bindings += desc;
        }
        
        VkDevice           &d = dev;
        /// create descriptor set layout
        auto desc_layout_info = VkDescriptorSetLayoutCreateInfo {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            nullptr, 0, uint32_t(bindings.size()), bindings.data() }; /// todo: fix general pipline. it will be different for Compute Pipeline, but this is reasonable for now if a bit lazy
        assert(vkCreateDescriptorSetLayout(d, &desc_layout_info, null, &set_layout) == VK_SUCCESS);
        
        /// of course, you do something else right about here...
        iDevice          &idev = dev.intern;
        const size_t  n_frames = idev.frames.size();
        auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, set_layout); /// add to vec.
        auto                ai = VkDescriptorSetAllocateInfo {};
        ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool      = idev.desc.pool;
        ai.descriptorSetCount  = n_frames;
        ai.pSetLayouts         = layouts.data();
        desc_sets.resize(n_frames);
        VkDescriptorSet *ds_data = desc_sets.data();
        VkResult res = vkAllocateDescriptorSets(d, &ai, ds_data);
        assert  (res == VK_SUCCESS);
        ubo.update(&dev);
        
        /// write descriptor sets for all swap image instances
        for (size_t i = 0; i < idev.frames.size(); i++) {
            auto &desc_set  = desc_sets[i];
            auto  v_writes  = array<VkWriteDescriptorSet>(size_t(1 + assets.size()));
            
            /// update descriptor sets
            v_writes       += ubo.descriptor(i, desc_set);
            for (auto &[r, tx]:assets)
                v_writes   += tx->descriptor(desc_set, uint32_t(r));
            
            size_t       sz = v_writes.size();
            VkWriteDescriptorSet *ptr = v_writes.data();
            vkUpdateDescriptorSets(d, uint32_t(sz), ptr, 0, nullptr);
        }

        ///
        str        cwd = Path::cwd();
        str     s_vert = var::format("{0}/shaders/{1}.vert", { cwd, shader });
        str     s_frag = var::format("{0}/shaders/{1}.frag", { cwd, shader });
        VAttribs vattr = vbo.fn_attribs();
        
        ///
        VkShaderModule vert,
                       frag;
        dev.module(s_vert, vattr, assets, Device::Vertex,   vert);
        dev.module(s_frag, vattr, assets, Device::Fragment, frag);

        ///
        array<VkPipelineShaderStageCreateInfo> stages {{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_VERTEX_BIT,   vert, shader.cstr(), null
        }, {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_FRAGMENT_BIT, frag, shader.cstr(), null
        }};

        ///
        auto binding            = VkVertexInputBindingDescription { 0, uint32_t(vsize), VK_VERTEX_INPUT_RATE_VERTEX };
        auto vk_attr            = vbo.fn_attribs();
        auto viewport           = VkViewport(dev);
        auto sc                 = VkRect2D {
                .offset             = { 0, 0 },
                .extent             = idev.extent };
        auto cba                = VkPipelineColorBlendAttachmentState {
                .blendEnable        = VK_FALSE,
                .colorWriteMask     = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
        auto layout_info        = VkPipelineLayoutCreateInfo {
                .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount     = 1,
                .pSetLayouts        = &set_layout
        };
        ///
        assert(vkCreatePipelineLayout(d, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);
        
        /// ok initializers here we go.
        vkState state {
            .vertex_info = {
                .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount   = 1,
                .pVertexBindingDescriptions      = &binding,
                .vertexAttributeDescriptionCount = uint32_t(vk_attr.size()),
                .pVertexAttributeDescriptions    = vk_attr.data()
            },
            .topology                    = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable  = VK_FALSE
            },
            .vs                          = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount           = 1,
                .pViewports              = &viewport,
                .scissorCount            = 1,
                .pScissors               = &sc
            },
            .rs                          = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable        = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode             = VK_POLYGON_MODE_FILL,
                .cullMode                = VK_CULL_MODE_FRONT_BIT,
                .frontFace               = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable         = VK_FALSE,
                .lineWidth               = 1.0f
            },
            .ms                          = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples    = idev.sampling,
                .sampleShadingEnable     = VK_FALSE
            },
            .ds                          = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable         = VK_TRUE,
                .depthWriteEnable        = VK_TRUE,
                .depthCompareOp          = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable   = VK_FALSE,
                .stencilTestEnable       = VK_FALSE
            },
            .blending                    = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable           = VK_FALSE,
                .logicOp                 = VK_LOGIC_OP_COPY,
                .attachmentCount         = 1,
                .pAttachments            = &cba,
                .blendConstants          = { 0.0f, 0.0f, 0.0f, 0.0f }
            }
        };
        ///
        VkGraphicsPipelineCreateInfo pi {
            .sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount                   = uint32_t(stages.size()),
            .pStages                      = stages.data(),
            .pVertexInputState            = &state.vertex_info,
            .pInputAssemblyState          = &state.topology,
            .pViewportState               = &state.vs,
            .pRasterizationState          = &state.rs,
            .pMultisampleState            = &state.ms,
            .pDepthStencilState           = &state.ds,
            .pColorBlendState             = &state.blending,
            .layout                       = pipeline_layout,
            .renderPass                   = idev.render_pass,
            .subpass                      = 0,
            .basePipelineHandle           = VK_NULL_HANDLE
        };
        ///
        vk_state(state);
        //if (vk_state)
       //     vk_state(state);
        
        assert (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
    }
};


struct iRender {
    Device            *dev;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes
    array<VkSemaphore> image_avail;      /// you are going to want ubo controller to update a lambda right before it transfers
    array<VkSemaphore> render_finish;    /// if we're lucky, that could adapt to compute model integration as well.
    array<VkFence>     fence_active;
    array<VkFence>     image_active;
    VkCommandBuffer    render_cmd;
    rgba               color  = {0,0,0,1};
    int                cframe = 0;
    int                sync   = 0;
    
    ///
    void push(PipelineData &pd) { sequence.push(&pd); }
    iRender(Device *dev = null): dev(dev) {
        if (dev) {
            VkDevice    &d = *dev;
            auto     &idev = *dev->intern;
            const Size ns  =  idev.swap_images.size(); /// !
            const Size mx  =  Render::max_frames;
            image_avail    =  array<VkSemaphore>(mx, VK_NULL_HANDLE);
            render_finish  =  array<VkSemaphore>(mx, VK_NULL_HANDLE);
            fence_active   =  array<VkFence>    (mx, VK_NULL_HANDLE);
            image_active   =  array<VkFence>    (ns, VK_NULL_HANDLE);
            
            ///
            VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
            ///
            for (size_t i = 0; i < mx; i++)
                assert (vkCreateSemaphore(d, &si, null,   &image_avail[i]) == VK_SUCCESS &&
                        vkCreateSemaphore(d, &si, null, &render_finish[i]) == VK_SUCCESS &&
                        vkCreateFence(    d, &fi, null,  &fence_active[i]) == VK_SUCCESS);
        }
    }
    
    /// the thing that 'executes' is the present.  so this is just 'update'
    void update() {
        Device    &dev = *this->dev;
        Frame   &frame = dev.intern->frames[cframe];
        assert (cframe == frame.index);
    }

    void present() {
        Device &dev = *this->dev;
        VkDevice &d = *this->dev;
        vkWaitForFences(dev, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
        
        uint32_t image_index; /// look at the ref here
        VkResult result = vkAcquireNextImageKHR(
            d, dev.intern->swap_chain, UINT64_MAX,
            image_avail[cframe], VK_NULL_HANDLE, &image_index);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            assert(false); // lets see when this happens
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            assert(false); // lets see when this happens
        ///
        if (image_active[image_index] != VK_NULL_HANDLE)
            vkWaitForFences(d, 1, &image_active[image_index], VK_TRUE, UINT64_MAX);
        ///
        vkResetFences(d, 1, &fence_active[cframe]);
        
        VkSemaphore          s_wait[] = {   image_avail[cframe] };
        VkSemaphore        s_signal[] = { render_finish[cframe] };
        VkPipelineStageFlags f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        auto              pipelines   = array<iPipelineData *>(sequence.size());
        Frame                &frame   = dev.intern->frames[cframe];
        const float               b   = 255.0f;
        auto            clear_color   = [&](rgba c) {
            return VkClearColorValue {{ float(c.r) / b, float(c.g) / b, float(c.b) / b, float(c.a) / b }};
        };
        
        /// transfer the ubo data on each and gather a VkCommandBuffer array to submit to vkQueueSubmit
        while (sequence.size()) {
            iPipelineData &m = *sequence.front()->intern;
            sequence.pop();
            /// send us your uniforms!
            m.ubo.transfer(image_index);
            pipelines += &m;
        }
        
        /// reallocate command
        VkCommandPool &pool = dev;
        vkFreeCommandBuffers(dev, pool, 1, &render_cmd);
        auto      alloc_info = VkCommandBufferAllocateInfo {
                    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    null, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        assert(vkAllocateCommandBuffers(dev, &alloc_info, &render_cmd) == VK_SUCCESS);
        
        /// begin command
        auto           binfo = VkCommandBufferBeginInfo {
                      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        
        assert(vkBeginCommandBuffer(render_cmd, &binfo) == VK_SUCCESS);
        
        auto         cvalues = array<VkClearValue> {
            {         .color = clear_color(color)}, // for image rgba  sfloat
            {  .depthStencil = {1.0f, 0}}};         // for image depth sfloat
        
        auto           rinfo = VkRenderPassBeginInfo { /// nothing in canvas showed with depthStencil = 0.0.
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext           = null,
            .renderPass      = VkRenderPass(d),
            .framebuffer     = VkFramebuffer(frame),
            .renderArea      = {{ 0, 0 }, dev.intern->extent},
            .clearValueCount = uint32_t(cvalues.size()),
            .pClearValues    = cvalues.data()
        };

        ///
        vkCmdBeginRenderPass(render_cmd, &rinfo, VK_SUBPASS_CONTENTS_INLINE);
        for (iPipelineData *p: pipelines)
            if (p->ibo) {
                Type index_type = p->ibo.buffer.type();
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindPipeline      (render_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline);
                vkCmdBindVertexBuffers (render_cmd, 0, 1, &(VkBuffer &)(p->vbo), offsets);
                vkCmdBindIndexBuffer   (render_cmd, p->ibo, 0, index_type.sz() == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(render_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->pipeline_layout, 0, 1, p->desc_sets.data(), 0, nullptr);
                vkCmdDrawIndexed       (render_cmd, uint32_t(p->ibo.size()), 1, 0, 0, 0);
            }
            
        vkCmdEndRenderPass(render_cmd);
        assert(vkEndCommandBuffer(render_cmd) == VK_SUCCESS);
        
        /// submit the render command...
        image_active[image_index]        = fence_active[cframe];
        VkSubmitInfo submit_info         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &render_cmd;
        submit_info.signalSemaphoreCount = 1;
        iDevice &idev = dev.intern;
        /// queues should not be on both dev and gpu (eliminate gpu)
        assert(vkQueueSubmit(idev.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
        ///
        VkPresentInfoKHR present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1, s_signal, 1, &idev.swap_chain, &image_index };
        VkResult         presult = vkQueuePresentKHR(idev.queues[GPU::Present], &present);
        
        if ((presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR)) {// && !sync_diff) {
            dev.update(); /// cframe needs to be valid at its current value
        }// else
         //   assert(result == VK_SUCCESS);
        
        cframe = (cframe + 1) % Render::max_frames;
    }
};

Frame::Frame(Device *dev): dev(dev) { }

///
void Frame::update() {
    Device   & dev  = *this->dev;
    VkDevice & d    =  dev;
    auto     & idev = *dev.intern;
    vkDestroyFramebuffer(d, framebuffer, nullptr);
    auto views = array<VkImageView>();
    for (Texture &tx: attachments)
        views += VkImageView(tx);
    VkFramebufferCreateInfo ci {};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = idev.render_pass;
    ci.attachmentCount = views.size();
    ci.pAttachments    = views.data();
    ci.width           = idev.extent.width;
    ci.height          = idev.extent.height;
    ci.layers          = 1;
    assert(vkCreateFramebuffer(d, &ci, nullptr, &framebuffer) == VK_SUCCESS);
}

///
void Frame::destroy() {
    VkDevice & device = *dev;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}

Frame::operator VkFramebuffer &() {
    return framebuffer;
}

void Asset::descriptor(Asset::Type t, VkDescriptorSetLayoutBinding &desc) {
    desc = VkDescriptorSetLayoutBinding
        { t, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, nullptr };
}

void Render::updating() {
    intern->sync++;
}

void Render::push(PipelineData &pd) { intern->sequence.push(&pd); }

// problem here with sz getting two different treatments. look at the header.





struct iTexture;
struct iBuffer {
    Device                 *dev;
    VkDescriptorBufferInfo  info;
    VkBuffer                buffer;
    VkDeviceMemory          memory;
    Type                    type;
    Size                    size;
    
    ///
    Size total_bytes()                  { return size * type.sz(); }
    operator VkDeviceMemory         &() { return memory; }
    operator VkBuffer               &() { return buffer; }
    operator VkDescriptorBufferInfo &() {
        return info;
    }
    
    iBuffer() : dev(null) { }
    /// impl: generic
    iBuffer(Device *dev, void *data, Type type, Size sz, VkBufferUsageFlags u, VkMemoryPropertyFlags p)
            : dev(dev), type(type), size(sz)
    {
        /// configure create-info
        VkDevice     &d = *dev;
        auto      &idev = *dev->intern;
        VkBuffer      b;
        auto          bi = VkBufferCreateInfo {};
        bi.sType         = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size          = size_t(size);
        bi.usage         = u;
        bi.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        /// create buffer
        assert(vkCreateBuffer(d, &bi, nullptr, &b) == VK_SUCCESS);
        
        /// fetch memory schema
        VkMemoryRequirements r;
        vkGetBufferMemoryRequirements(d, b, &r);

        /// allocate and bind with memory schema
        VkDeviceMemory       m;
        VkMemoryAllocateInfo a {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = r.size,
            .memoryTypeIndex = idev.memory_type(r.memoryTypeBits, p)
        };
        assert(vkAllocateMemory(d, &a, nullptr, &m) == VK_SUCCESS);
        
        vkBindBufferMemory(d, b, (VkDeviceMemory &)data, 0);
        
        ///
        info = VkDescriptorBufferInfo {
            .buffer = b,
            .offset = 0,
            .range  = size_t(size) /// was:sizeof(UniformBufferObject)
        };

        /// copy handle
        buffer = b;
        memory = m;
    }
    
    bool operator==(iBuffer &b) const { return this == &b; }
    bool operator!=(iBuffer &b) const { return !(operator==(b)); }
    
    void allocate(VkBufferUsageFlags &u) {
        VkDevice      &d = *dev;
        auto          bi =  VkBufferCreateInfo {
            .sType       =  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        =  size_t(size),
            .usage       =  u,
            .sharingMode =  VK_SHARING_MODE_EXCLUSIVE };
        ///
        info = VkDescriptorBufferInfo {
            .buffer      =  buffer,
            .offset      =  0,
            .range       =  size_t(size)
        };
        ///
        assert(vkCreateBuffer(d, &bi, nullptr, &buffer) == VK_SUCCESS);
    }

    void transfer(void *v) {
        VkDevice &d = *dev;
        void*     mapped;
        size_t    nbytes = total_bytes();
          vkMapMemory(d, memory, 0, nbytes, 0, &mapped);
               memcpy(mapped, v, nbytes);
        vkUnmapMemory(d, memory);
    }

    /// no crop support yet; simple
    void copy_to(iTexture *itx);

    void copy_to(Buffer &dst, size_t size) {
        dev->command([&](VkCommandBuffer &cmd) {
            VkBufferCopy params {};
            params.size = size;
            vkCmdCopyBuffer(cmd, *this, dst, 1, &params);
        });
    }



    void destroy() {
        VkDevice       &d = *dev;
        ///
        vkDestroyBuffer(d, buffer, null);
        vkFreeMemory   (d, memory, null);
    }
};

/// Buffer interface to backend
Buffer::Buffer(std::nullptr_t n) { }
Buffer::Buffer(Device *dev, void *data, Type t, Size sz, VkBufferUsageFlags u, VkMemoryPropertyFlags p) :
    intern(new iBuffer(dev, data, t, sz, u, p)) { }

///
Size    Buffer::total_bytes()                      { return intern->total_bytes(); }
Size    Buffer::size       ()                      { return intern->size;   }
Type    Buffer::type       ()                      { return intern->type;   }
void    Buffer::destroy    ()                      { intern->destroy();     }
void    Buffer::allocate   (VkBufferUsageFlags &u) { intern->allocate(u);   }
void    Buffer::transfer   (void *v)               { intern->transfer(v);   }
void    Buffer::copy_to    (Buffer &b, size_t s)   { intern->copy_to(b, s); }
void    Buffer::copy_to    (iTexture *t)           { intern->copy_to(t); }
Buffer &Buffer::operator=  (const Buffer &r)       {
    if (this != &r)
        intern = (sh<iBuffer> &)r.intern;
    return *this;
}

///
Buffer::operator               VkBuffer &() { return (VkBuffer               &)intern; }
Buffer::operator         VkDeviceMemory &() { return (VkDeviceMemory         &)intern; }
Buffer::operator VkDescriptorBufferInfo &() { return (VkDescriptorBufferInfo &)((iBuffer &)intern); }





/// surface handle passed in for all gpus, its going to be the same one for each
GPU::GPU(VkPhysicalDevice &gpu, VkSurfaceKHR &surface) : intern(new iGPU { gpu, surface }) { }

GPU &GPU::operator=(const GPU &gpu) {
    if (this != &gpu)
        intern = (sh<iGPU> &)gpu.intern;
    return *this;
}

array<GPU> GPU::listing() {
    VkInstance vk = Vulkan::instance();
    uint32_t   gpu_count;
    vkEnumeratePhysicalDevices(vk, &gpu_count, null);
    auto      gpu = array<GPU>();
    auto       hw = array<VkPhysicalDevice>();
    gpu.resize(gpu_count);
    hw.resize(gpu_count);
    vkEnumeratePhysicalDevices(vk, &gpu_count, hw.data());
    vec2i sz = Vulkan::startup_rect().size();
    VkSurfaceKHR surf;
    Vulkan::surface(sz, surf);
    for (size_t i = 0; i < gpu_count; i++)
        gpu[i] = GPU(hw[i], surf);
    return gpu;
}


         GPU::operator VkPhysicalDevice &()   { return (VkPhysicalDevice &)*intern;  }
         GPU::operator bool ()                { return intern->supported();          }
bool     GPU::has_capability(Capability caps) { return intern->has_capability(caps); }
uint32_t GPU::index         (Capability caps) { return intern->index(caps);          }
GPU      GPU::select        (int index)       { return GPU::listing()[index];        }

///
/// ------------------------------ UniformData ------------------------------ ///
///

/// obtain write desciptor set
VkWriteDescriptorSet UniformData::descriptor(size_t f_index, VkDescriptorSet &ds) {
    auto &m = *this->m;
    return {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, null, ds, 0, 0,
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, null, &(VkDescriptorBufferInfo &)m.buffers[f_index], null
    };
}

/// lazy initialization of Uniforms make the component tree work a bit better
void UniformData::update(Device *dev) {
    auto &m = *this->m;
    //for (auto &b: m.buffers)
    //    b.destroy();
    m.dev = dev;
    size_t n_frames = dev->intern->frames.size();
    m.buffers = array<Buffer>(n_frames);
    for (int i = 0; i < n_frames; i++)
        m.buffers += Buffer {
            dev, null, Type(), Size(m.struct_sz), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
    }

void UniformData::destroy() {
    auto &m = *this->m;
    size_t n_frames = m.dev->intern->frames.size();
    for (int i = 0; i < n_frames; i++)
        m.buffers[i].destroy();
}

/// called by the Render
void UniformData::transfer(size_t frame_id) {
    auto        &m = *this->m;
    VkDevice  &dev = *m.dev;
    Buffer &buffer = m.buffers[frame_id]; /// todo -- UniformData never initialized (problem, that is)
    size_t  nbytes = buffer.total_bytes();
    cchar_t *data[2048];
    void* gpu;
    memset(data, 0, sizeof(data));
    m.fn(data);
    vkMapMemory(dev, buffer, 0, nbytes, 0, &gpu);
    memcpy(gpu, data, nbytes);
    vkUnmapMemory(dev, buffer);
}

///
/// ------------------------------ Render ------------------------------ ///
///

        Render::Render   (Device  *d)      : intern(new iRender(d)) { }
      //Render::Render   (const Render &r) : intern(r.intern) { }
Render &Render::operator=(const Render &r) {
    if (this  != &r)
        intern =  (sh<iRender> &)r.intern;
    return *this;
}
void      Render::update   () { intern->update();  }
void      Render::present  () { intern->present(); }
const int Render::max_frames = 2;

///
/// ------------------------------ Device ------------------------------ ///
///

void Device::command(std::function<void(VkCommandBuffer &)> fn) {
    intern->command(fn);
}

ResourceCache &Device::get_cache() { return intern->resource_cache; }; /// ::map<Path, Device::Pair>

static Device d_null;
///
Device::Device(nullptr_t n)   { }
Device::Device(GPU &gpu, bool aa) : intern(new iDevice(this, gpu, aa)) { }
///
void     Device::push(PipelineData &pd) { intern->render.push(pd); }
Device  &Device::null_device() { return d_null; }
Device  &Device::sync()        { intern->sync(); return *this; }
void     Device::module(Path path, VAttribs &vattr, Assets &assets, Module type, VkShaderModule &module) {
    intern->module(path, vattr, assets, type, module);
}
///
uint32_t Device::memory_type(uint32_t types, VkMemoryPropertyFlags props)
                                  { return intern->memory_type(types, props); }
///
void     Device::update()                { intern->update(); }
void     Device::initialize(Window *win) { intern->initialize(win); }
///

Device::operator VkViewport                             &() { return (VkViewport                             &)*intern; }
Device::operator VkPipelineViewportStateCreateInfo      &() { return (VkPipelineViewportStateCreateInfo      &)*intern; }
Device::operator VkPipelineRasterizationStateCreateInfo &() { return (VkPipelineRasterizationStateCreateInfo &)*intern; }

/// interface required for initialization of Skia canvas (perhaps not if default?)
         Device::operator VkPhysicalDevice &() { return (VkPhysicalDevice &)*intern; }
         Device::operator VkCommandPool    &() { return (VkCommandPool    &)*intern; }
         Device::operator VkDevice         &() { return (VkDevice         &)*intern; }
         Device::operator VkRenderPass     &() { return (VkRenderPass     &)*intern; }
VkQueue &Device::queue(GPU::Capability cap)    { return (VkQueue &)intern->queues[cap]; }
void     Device::destroy()                     { intern->destroy(); }
Device  &Device::operator=(const Device &dev)  {
    if (this != &dev)
        intern = (sh<iDevice> &)dev.intern;
    return *this;
}

///
/// ------------------------------ VertexData ------------------------------ ///
///

/// Vertex / VertexData
VAttribs Vertex::attribs(FlagsOf<Attr> attr) {
    auto   res = VAttribs(6);
    if (attr[Position])  res += { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  pos)  };
    if (attr[Normal])    res += { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  norm) };
    if (attr[UV])        res += { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex,  uv)   };
    if (attr[Color])     res += { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex,  clr)  };
    if (attr[Tangent])   res += { 4, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  ta)   };
    if (attr[BiTangent]) res += { 5, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  bt)   };
    return res;
}

VkWriteDescriptorSet &VertexData::operator()(VkDescriptorSet &ds) {
    wds = new VkWriteDescriptorSet {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, (VkDescriptorType &)buffer };
    return wds;
}

///
VertexData &VertexData::operator=(const VertexData &r) {
    if (this != &r) {
        dev        = r.dev;
        buffer     = r.buffer;
        fn_attribs = r.fn_attribs;
        wds        = (sh<VkWriteDescriptorSet> &)r.wds;
    }
    return *this;
}

VertexData::VertexData(std::nullptr_t n) { }
VertexData::VertexData(Device &dev, Buffer buffer, std::function<VAttribs()> fn_attribs) :
            dev(&dev), buffer(buffer), fn_attribs(fn_attribs) { }
///
        VertexData::operator VkBuffer &() { return buffer;         }
size_t  VertexData::size               () { return buffer.total_bytes(); } /// this could be wrong, it was size(), check use-case
        VertexData::operator bool      () { return dev != null; }
bool    VertexData::operator!          () { return dev == null; }


///
/// ------------------------------ iTexture ------------------------------ ///
///

struct iTexture {
    Device                 *dev        = null;
    VkImage                 image      = VK_NULL_HANDLE;
    VkImageView             view       = VK_NULL_HANDLE;
    VkDeviceMemory          memory     = VK_NULL_HANDLE;
    VkDescriptorImageInfo   info       = {};
    VkSampler               sampler    = VK_NULL_HANDLE;
    vec2i                   sz         = { 0, 0 };
    Asset::Type             asset_type = Asset::Undefined;
    int                     mips       = 0;
    VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageTiling           tiling     = VK_IMAGE_TILING_OPTIMAL;
    bool                    ms         = false;
    bool                    image_ref  = false;
    VkFormat                format;
    VkImageUsageFlags       usage;
    VkImageAspectFlags      aflags;
    Image                   lazy;
    ///
    std::stack<Texture::Stage> stage;
    Texture::Stage             prv_stage       = Texture::Stage::Undefined;
    VkImageLayout              layout_override = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAttachmentReference      aref;
    VkAttachmentDescription    adesc;
    ///
    iTexture(std::nullptr_t n = null) { }

    /// create with solid color
    iTexture(Device *dev, vec2i sz, rgba clr,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms,
                    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, int mips = -1, Image lazy = null) :
                        dev(dev), sz(sz),
                        mips(auto_mips(mips, sz)), mprops(m),
                        ms(ms), format(format), usage(u), aflags(a), lazy(lazy) { }

    /// create with image (rgba required)
    iTexture(Device *dev, Image &im, Asset::Type t,
                        VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                        VkImageAspectFlags a, bool ms,
                        VkFormat           f = VK_FORMAT_R8G8B8A8_UNORM, int  mips = -1) :
                        dev(dev), sz(im.size()), asset_type(t), mips(auto_mips(mips, sz)),
                        mprops(m),  ms(ms),  format(f),  usage(u), aflags(a),  lazy(im) { }

    iTexture(Device *dev, vec2i sz, VkImage image, VkImageView view,
                        VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                        VkImageAspectFlags a, bool ms, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, int mips = -1):
                           dev(dev),        image(image),                 view(view),
                            sz(sz),          mips(auto_mips(mips, sz)), mprops(m),
                            ms(ms),     image_ref(true),                format(format),
                            usage(u),      aflags(a) { }
    
    void transfer(VkCommandBuffer &cmd, VkBuffer &buffer) {
        auto               cp = VkBufferImageCopy {
            .imageSubresource = { aflags, 0, 0, 1 },
            .imageExtent      = { uint32_t(sz.x), uint32_t(sz.y), 1 }
        };
        vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
    }
    
    void transfer_pixels(rgba *pixels) {
        create_resources();
        if (pixels) {
            /// can make use of Type directly
            Type        id = Id<rgba>();
            Size     bsize = VkDeviceSize(sz.x * sz.y); /// definitely going to encounter some issues here
            Buffer staging = Buffer(dev, pixels, id, bsize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            ///
            void*      mem = null;
            size_t  nbytes = bsize * id.sz();
            assert(staging.total_bytes() == nbytes);
            vkMapMemory(*dev, staging, 0, nbytes, 0, &mem);
            memcpy(mem, pixels, size_t(nbytes));
            vkUnmapMemory(*dev, staging);
            ///
            push_stage(Texture::Stage::Transfer);
            staging.copy_to(this);
            //pop_stage(); // ah ha!..
            staging.destroy();
        }
        ///if (mip > 0) # skipping this for now
        ///    generate_mipmaps(device, image, format, sz.x, sz.y, mip);
    }

    
    void create_sampler() {
        VkDevice         &d   = *dev;
        VkPhysicalDevice &gpu = *dev;
        VkPhysicalDeviceProperties props {};
        vkGetPhysicalDeviceProperties(gpu, &props);
        ///
        VkSamplerCreateInfo si {};
        si.sType                     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.magFilter                 = VK_FILTER_LINEAR;
        si.minFilter                 = VK_FILTER_LINEAR;
        si.addressModeU              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.addressModeV              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.addressModeW              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.anisotropyEnable          = VK_TRUE;
        si.maxAnisotropy             = props.limits.maxSamplerAnisotropy;
        si.borderColor               = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        si.unnormalizedCoordinates   = VK_FALSE;
        si.compareEnable             = VK_FALSE;
        si.compareOp                 = VK_COMPARE_OP_ALWAYS;
        si.mipmapMode                = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.minLod                    = 0.0f;
        si.maxLod                    = float(mips);
        si.mipLodBias                = 0.0f;
        ///
        VkResult r = vkCreateSampler(d, &si, nullptr, &sampler);
        assert  (r == VK_SUCCESS);
    }

    /// see how the texture is created elsewhere with msaa
    void create_resources() {
        Device &dev = *this->dev;
        VkDevice &d = dev;
        VkImageCreateInfo imi {};
        ///
        layout_override             = VK_IMAGE_LAYOUT_UNDEFINED;
        ///
        imi.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imi.imageType               = VK_IMAGE_TYPE_2D;
        imi.extent.width            = uint32_t(sz.x);
        imi.extent.height           = uint32_t(sz.y);
        imi.extent.depth            = 1;
        imi.mipLevels               = mips;
        imi.arrayLayers             = 1;
        imi.format                  = format;
        imi.tiling                  = tiling;
        imi.initialLayout           = layout_override;
        imi.usage                   = usage;
        imi.samples                 = dev.intern->sampling;
        imi.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
        ///
        assert(vkCreateImage(d, &imi, nullptr, &image) == VK_SUCCESS);
        ///
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(d, image, &req);
        ///
        VkMemoryAllocateInfo a {};
        a.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        a.allocationSize            = req.size;
        a.memoryTypeIndex           = dev.memory_type(req.memoryTypeBits, mprops); /// this should work; they all use the same mprops from what i've seen so far and it stores that default
        assert(vkAllocateMemory(d, &a, nullptr, &memory) == VK_SUCCESS);
        vkBindImageMemory(d, image, memory, 0);
        ///
        if (ms)
            create_sampler();
        ///
        // run once like this:
        //set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); /// why?
    }

    void pop_stage() {
        assert(stage.size() >= 2);
        /// kind of need to update it.
        stage.pop();
        set_stage(stage.top());
    }

    void set_stage(Texture::Stage next_stage) {
        Device &       dev = *this->dev;
        Texture::Stage cur = prv_stage;
        ///
        while (cur != next_stage) {
            Texture::Stage  from = cur;
            Texture::Stage  to   = cur;
            /// skip over that color thing because this is quirky
            do to = Texture::Stage::Type(int(to) + ((next_stage > prv_stage) ? 1 : -1));
            while (to == Texture::Stage::Color && to != next_stage);
            /// transition through the image membrane, as if we were insane in-the
            Texture::Stage::Data      data = from.data();
            Texture::Stage::Data next_data =   to.data();
            VkImageMemoryBarrier   barrier = {
                .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout                       =      data.layout,
                .newLayout                       = next_data.layout,
                .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
                .image                           = image,
                .subresourceRange                = {
                    .aspectMask                  = aflags,
                    .baseMipLevel                = 0,
                    .levelCount                  = uint32_t(mips),
                    .baseArrayLayer              = 0,
                    .layerCount                  = 1
                },
                .srcAccessMask                   = VkAccessFlags(     data.access),
                .dstAccessMask                   = VkAccessFlags(next_data.access)
            };
            dev.command([&](VkCommandBuffer &cb) {
                vkCmdPipelineBarrier(
                    cb, data.stage, next_data.stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            });
            cur = to;
        }
        prv_stage = next_stage;
    }

    void push_stage(Texture::Stage next_stage) {
        if (stage.size() == 0)
            stage.push(Texture::Stage::Undefined); /// default stage, when pop'd
        if (next_stage == stage.top())
            return;
        
        set_stage(next_stage);
        stage.push(next_stage);
    }

    VkImageView create_view(VkDevice device, vec2i &sz, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
        VkImageView  view;
        uint32_t      mips = auto_mips(mip_levels, sz);
        //auto         usage = VkImageViewUsageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, null, VK_IMAGE_USAGE_SAMPLED_BIT };
        auto             v = VkImageViewCreateInfo {};
        v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //v.pNext            = &usage;
        v.image            = image;
        v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        v.format           = format;
        v.subresourceRange = { aspect_flags, 0, mips, 0, 1 };
        assert (vkCreateImageView(device, &v, nullptr, &view) == VK_SUCCESS);
        return view;
    }
    ///
    int             auto_mips(uint32_t mips, vec2i sz) { return 1; }
    ///
    operator            bool () { return image != VK_NULL_HANDLE; }
    bool            operator!() { return image == VK_NULL_HANDLE; }
    operator        VkImage &() { return image;  }
    operator VkDeviceMemory &() { return memory; }
    
    VkImageView image_view() {
        if (!view)
             view = create_view(*dev, sz, image, format, aflags, mips);
        return view;
    }

    VkSampler image_sampler() {
        if (!sampler) // random memory
            create_sampler();
        return sampler;
    }
    
    operator    VkImageView &() {
        if (!view)
             view = create_view(*dev, sz, image, format, aflags, mips); /// hide away the views, only create when they are used
        return view;
    }
    ///
    operator VkAttachmentDescription &() {
        assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
               usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        adesc = VkAttachmentDescription {
            .format         = format,
            .samples        = dev->intern->sampling,
            .loadOp         = image_ref ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                          VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        =  is_color ? VK_ATTACHMENT_STORE_OP_STORE :
                                          VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = image_ref  ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                              is_color   ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
        return adesc;
    }

    /// attach member memory inside of functions for more isolation potential
    /// mix this with Component (node)
    /// this would be like saying: install a method
    
    /*
    auto method = TMethod <Class1, Class2> { [] (int arg, int arg2, str &obj) {
        int member;
    }};
     */
    
    operator VkAttachmentReference &() {
        assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
               usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        //bool  is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ///  if we only do things once, do it once.
        auto is_referencing = [&](iTexture *a_data, iTexture *b_data) {
            if (!a_data && !b_data)
                return false;
            if (a_data && b_data && a_data->image && a_data->image == b_data->image)
                return true;
            if (a_data && b_data && a_data->view  && a_data->view  == b_data->view)
                return true;
            return false;
        };

        bool found = false;
        iDevice &idev = dev->intern;
        for (auto &f: idev.frames)
            for (int att_i = 0; att_i < f.attachments.size(); att_i++)
                if (is_referencing(this, (iTexture *)f.attachments[att_i].intern)) {
                    aref = VkAttachmentReference {
                        .attachment  = uint32_t(att_i),
                        .layout      = (layout_override != VK_IMAGE_LAYOUT_UNDEFINED) ? layout_override :
                                    (stage.size() ? stage.top().data().layout : VK_IMAGE_LAYOUT_UNDEFINED)
                    };
                    found = true;
                    break;
                }
        return aref;
    }

    operator VkDescriptorImageInfo &() {
        info  = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = image_view(),
            .sampler     = image_sampler()
        };
        return info;
    }

    void destroy() {
        if (dev) {
            vkDestroyImageView(*dev, view,   nullptr);
            vkDestroyImage(*dev,     image,  nullptr);
            vkFreeMemory(*dev,       memory, nullptr);
        }
    }

    VkWriteDescriptorSet descriptor(VkDescriptorSet &ds, uint32_t binding) {
        VkDescriptorImageInfo &info = *this;
        return {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = ds,
            .dstBinding      = binding,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo      = &info
        };
    }
};

Texture::Stage::Stage(Stage::Type value) : value(value) { }

const Texture::Stage::Data Texture::Stage::types[5] = {
    { Texture::Stage::Undefined, VK_IMAGE_LAYOUT_UNDEFINED,                0,                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { Texture::Stage::Transfer,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
    { Texture::Stage::Shader,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  },
    { Texture::Stage::Color,     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { Texture::Stage::Depth,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
};

Texture &Texture::operator=(const Texture &ref) {
    if (this != &ref)
        intern = (sh<iTexture> &)ref.intern;
    return *this;
}

void Texture::transfer(VkCommandBuffer &cmd, VkBuffer &buffer) {
    intern->transfer(cmd, buffer);
}

iTexture *Texture::get_data() { return intern; }

Texture::Texture(std::nullptr_t n) { }
Texture::Texture(Device            *device,  vec2i sz,        rgba clr,
        VkImageUsageFlags  usage,   VkMemoryPropertyFlags memory,
        VkImageAspectFlags aspect,  bool                  ms,
        VkFormat           format,  int mips, Image lazy) :
    intern(new iTexture { device, sz, clr, usage, memory,
                    aspect, ms, format, mips, lazy })
{
    rgba *px = null;
    if (clr) { /// this also works to be the initial color too if its a url; its designed to update async within reason
        px = (rgba *)malloc(sz.x * sz.y * sizeof(rgba));
        for (int i = 0; i < sz.x * sz.y; i++)
            px[i] = clr;
    }
    transfer_pixels(px); /// this was made to be null.. what i wonder is how its done
    free(px);
    intern->image_ref = false;
}

Texture::Texture(Device *device, Image &im, Asset::Type t) {
    VkMemoryPropertyFlags memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageAspectFlags    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkFormat              format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageUsageFlags      usage = VK_IMAGE_USAGE_SAMPLED_BIT      | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
                                    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    intern = new iTexture {
        device, im, t, usage, memory, aspect, false, format, -1
    };
    rgba *px = &im.pixel<rgba>(0, 0);
    transfer_pixels(px);
    free(px);
    intern->image_ref = false;
}

Texture::Texture(Device            *device, vec2i                 sz,
        VkImage            image,  VkImageView           view,
        VkImageUsageFlags  usage,  VkMemoryPropertyFlags memory,
        VkImageAspectFlags aspect, bool                  ms,
        VkFormat           format, int mips) :
    intern(new iTexture {
        device, sz, image, view, usage, memory, aspect, ms, format, mips
    }) { }


/// this module ripples with unorthogonalities, for her pleasure.

void Texture::set_stage(Stage s)              { return intern->set_stage(s);  }
void Texture::push_stage(Stage s)             { return intern->push_stage(s); }
void Texture::pop_stage()                     { return intern->pop_stage();   }
void Texture::transfer_pixels(rgba *pixels)   { return intern->transfer_pixels(pixels); }
Texture::operator  bool                    () { return    intern &&  *intern; }
bool Texture::operator!                    () { return   !intern || !*intern; }
bool Texture::operator==        (Texture &tx) { return    intern &&   intern == tx.intern; }
Texture::operator VkImage                 &() { return   *intern; }
Texture::operator VkImageView             &() { return   *intern; }
Texture::operator VkDeviceMemory          &() { return   *intern; }
Texture::operator VkAttachmentReference   &() { return   *intern; }
Texture::operator VkAttachmentDescription &() { return   *intern; }
Texture::operator VkDescriptorImageInfo   &() { return   *intern; }

void Texture::destroy() { if (intern) intern->destroy(); };
VkWriteDescriptorSet Texture::descriptor(VkDescriptorSet &ds, uint32_t binding) {
    return intern->descriptor(ds, binding);
}


void iBuffer::copy_to(iTexture *itx) {
    dev->command([&](VkCommandBuffer &cmd) {
        itx->transfer(cmd, buffer);
    });
}


typedef map<Asset::Type, Texture *> Assets;

ColorTexture::ColorTexture(Device *device, vec2i sz, rgba clr):
    Texture(device, sz, clr,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_UNORM, 1) { // was VK_FORMAT_B8G8R8A8_SRGB
        intern->image_ref = false; /// 'SwapImage' should have this set as its a View-only.. [/creeps away slowly, very unsure of themselves]
        intern->push_stage(Stage::Color);
    }

DepthTexture::DepthTexture(Device *device, vec2i sz, rgba clr):
    Texture(device, sz, clr,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT, true, VK_FORMAT_D32_SFLOAT, 1) {
        intern->image_ref = false; /// we are explicit here.. just plain explicit.
        intern->push_stage(Stage::Depth);
    }

SwapImage::SwapImage(Device *device, vec2i sz, VkImage image):
    Texture(device, sz, image, null,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_B8G8R8A8_UNORM, 1) { // VK_FORMAT_B8G8R8A8_UNORM tried here, no effect on SwapImage
        intern->layout_override = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

static recti s_rect;

/// allocate composer, the class which takes rendered elements and manages instances
/// allocate window internal (glfw subsystem) and canvas (which initializes a Device for usage on skia?)
int Vulkan::main(Composer *composer) {
    Map      &args = composer->args;
    vec2i       sz = { args.count("window-width")  ? int(args["window-width"])  : 512,
                       args.count("window-height") ? int(args["window-height"]) : 512 };
            s_rect = recti { 0, 0, sz.x, sz.y };
    Internal    &i = Internal::handle();
    Window      &w = *i.win;
    Device    &dev =  i.dev;
    Canvas  canvas = Canvas(sz, Canvas::Context2D);
    Texture &tx_canvas = i.tx_skia;
    
    ///
    composer->sz = &w.size;
    composer->ux->canvas = canvas;
    
    ///
    i.dev.initialize(&w);
    w.show();
    
    auto   vertices =       Vertex::square ();
    auto    indices =       array<uint16_t> { 0, 1, 2, 2, 3, 0 };
    auto   textures =               Assets {{ Asset::Color, &tx_canvas }};
    auto      vattr =      Vertex::Attribs  { Vertex::Position, Vertex::UV, Vertex::Normal };
    auto        vbo =  VertexBuffer<Vertex> { dev, vertices, vattr };
    auto        ibo = IndexBuffer<uint16_t> { dev, indices };
    auto        uni =    UniformBuffer<MVP> { dev, [&](MVP &mvp) {
        mvp         = MVP {
             .model = m44f::identity(),
             .view  = m44f::identity(),
             .proj  = m44f::ortho(-0.5f, 0.5f, -0.5f, 0.5f, {0.5f, -0.5f})
        };
    }};
    auto         pl = Pipeline<Vertex> { dev, uni, vbo, ibo, textures, std::string("main") };
    
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

        canvas.clear(rgba {0.0, 1.0, 0.0, 1.0});
        canvas.color(rgba {1.0, 1.0, 1.0, 1.0});
        rectd r = rectd { 0, 0, 320, 240 };
        canvas.text("hello world", r, {0.5, 0.5}, {0.0, 0.0}, false);
        composer->render();
        canvas.flush();
    
        // this should only paint the 3D pipeline, no canvas
        iDevice &idev = dev.intern;
        i.tx_skia.push_stage(Texture::Stage::Shader);
        idev.render.push(pl); /// Any 3D object would have passed its pipeline prior to this call (in render)
        idev.render.present();
        i.tx_skia.pop_stage();
        
        if (composer->root)
            w.set_title(composer->root->m.text.label);

        composer->process();
        //glfwWaitEventsTimeout(1.0);
    });
    return 0;
}



PipelineData::operator bool  ()                 { return  intern->enabled;    }
bool PipelineData::operator! ()                 { return !intern->enabled;    }
void PipelineData::  enable  (bool en)          { intern->enabled = en;       }
bool PipelineData::operator==(PipelineData &b)  { return intern == b.intern;  }
PipelineData::PipelineData   (std::nullptr_t n) {                             }
void PipelineData::update    (size_t frame_id)  { }

///
PipelineData::PipelineData(Device &device, UniformData &ubo,   VertexData &vbo,    IndexData &ibo,
                           Assets &assets, size_t       vsize, string      shader, VkStateFn  vk_state) :
                    intern(new iPipelineData(device, ubo, vbo, ibo, assets, vsize, shader, vk_state)) { }



static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessageTypeFlagsEXT             type,
            const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
            void*                                       user_data) {
    std::cerr << "validation layer: " << cb_data->pMessage << std::endl;

    return VK_FALSE;
}

void vk_subsystem_init() {
    static std::atomic<bool> init;
    if (!init) {
        init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
}

Internal &Internal::handle()    { return _.vk ? _ : _.bootstrap(); }
Internal &Internal::bootstrap() {
    vk_subsystem_init();
    
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    layers.resize(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    ///
    VkApplicationInfo appInfo {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "ion";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "ion";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_MAKE_VERSION(1, 1, 0);//VK_API_VERSION_1_0; -- this seems to fix
    ///
    VkInstanceCreateInfo ci {};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ///
    uint32_t    n_ext;
    auto          dbg = VkDebugUtilsMessengerCreateInfoEXT {};
    auto     glfw_ext = (cchar_t **)glfwGetRequiredInstanceExtensions(&n_ext); /// this is ONLY for instance; must be moved.
    auto instance_ext = array<cchar_t *>(n_ext + 1);
    for (int i = 0; i < n_ext; i++)
        instance_ext += glfw_ext[i];
    instance_ext     += "VK_KHR_get_physical_device_properties2";
    #if !defined(NDEBUG)
        instance_ext          += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        ci.enabledLayerCount   = uint32_t(validation_layers.size());
        ci.ppEnabledLayerNames = (cchar_t* const*)validation_layers.data();
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
    ci.ppEnabledExtensionNames = instance_ext.data();
    auto res                   = vkCreateInstance(&ci, nullptr, &vk);
    assert(res == VK_SUCCESS);
    ///
    gpu = GPU::select();
    dev = Device(gpu, true);
    return *this;
}

Texture Vulkan::texture(vec2i size) {
    auto &i = Internal::handle();
    i.tx_skia.destroy();
    i.tx_skia = Texture { &i.dev, size, rgba { 1.0, 1.0, 1.0, 1.0 },
        VK_IMAGE_USAGE_SAMPLED_BIT          | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT     | VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
        false, VK_FORMAT_R8G8B8A8_UNORM, -1 };
    i.tx_skia.push_stage(Texture::Stage::Color);
    return i.tx_skia; /// created for skia, likely SRGB change causing incompatibility internally; we never tested that after the fix to the color issue seen
}

///
recti Vulkan::startup_rect() {
    assert(s_rect);
    return s_rect;
}

void Vulkan::surface(vec2i &sz, VkSurfaceKHR &result) {
    Internal &i = Internal::handle();
    if (!i.win) {
         i.win = new Window(sz);
         i.win->fn_resize = [i=i]() { ((Internal &)i).resize = true; };
    }
    result = *i.win;
}

void              Vulkan::init       () {        Internal::handle(); }
uint32_t          Vulkan::queue_index() { return Internal::handle().dev.intern->gpu.index(GPU::Graphics); }
uint32_t          Vulkan::version    () { return VK_MAKE_VERSION(1, 1, 0); }
VkInstance       &Vulkan::instance   () { return Internal::handle().vk; }
Device           &Vulkan::device     () { return Internal::handle().dev; }
VkPhysicalDevice &Vulkan::gpu        () { return Internal::handle().dev; }
VkQueue          &Vulkan::queue      () { return Internal::handle().dev.queue(GPU::Graphics); }
