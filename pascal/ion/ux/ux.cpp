//#include <core/core.hpp>
//#include <math/math.hpp>
//#include <async/async.hpp>
//#include <image/image.hpp>
#include <ux/ux.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vkvg.h>
#ifdef __APPLE__
#include <vulkan/vulkan_metal.h>
#endif
#include <array>
#include <set>
#include <stack>
#include <queue>
#include <vkh.h>
#include <vulkan/vulkan.h>


template <> struct is_opaque<VkImageView>       : true_type { };
template <> struct is_opaque<VkPhysicalDevice>  : true_type { };
template <> struct is_opaque<VkImage>           : true_type { };
template <> struct is_opaque<VkFence>           : true_type { };
template <> struct is_opaque<VkSemaphore>       : true_type { };
template <> struct is_opaque<VkBuffer>          : true_type { };
template <> struct is_opaque<VkCommandBuffer>   : true_type { };
template <> struct is_opaque<VkDescriptorSet>   : true_type { };
template <> struct is_opaque<VkLayerProperties> : true_type { };

style::entry *prop_style(node &n, prop *member) {
    style           *st = (style *)n.fetch_style();
    mx         s_member = member->key;
    array<style::block> &blocks = st->members(s_member); /// instance function when loading and updating style, managed map of [style::block*]
    style::entry *match = null; /// key is always a symbol, and maps are keyed by symbol
    real     best_score = 0;
    ///
    /// find top style pair for this member
    for (style::block &block:blocks) {
        real score = block.match(&n);
        if (score > 0 && score >= best_score) {
            match = block.b_entry(member->key);
            best_score = score;
        }
    }
    
    return match;
}

using VkAttribs = array<VkVertexInputAttributeDescription>;

VkDescriptorSetLayoutBinding Asset_descriptor(Asset::etype t) {
    return { uint32_t(t), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, nullptr };
}

/// VkAttribs (array of VkAttribute) [data] from VAttribs (state flags) [model]
void Vertex::attribs(VAttribs attr, void *res) {
    VkAttribs &vk_attr = (*(VkAttribs*)res = VkAttribs(6));
    if (attr[VA::Position])  vk_attr += { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  pos)  };
    if (attr[VA::Normal])    vk_attr += { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  norm) };
    if (attr[VA::UV])        vk_attr += { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex,  uv)   };
    if (attr[VA::Color])     vk_attr += { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex,  clr)  };
    if (attr[VA::Tangent])   vk_attr += { 4, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  ta)   };
    if (attr[VA::BiTangent]) vk_attr += { 5, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  bt)   };
}

struct VertexInternal {
    Device         *device = null;
    Buffer          buffer;
    lambda<void(void*)> fn_attribs;
    
    //VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
    //    return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    //}
    VertexInternal() { }
    VertexInternal(Device *device, Buffer buffer, lambda<void(void*)> fn_attribs) :
        device(device), buffer(buffer), fn_attribs(fn_attribs) { }
};

VertexData::VertexData(Device *device, Buffer buf, lambda<void(void*)> fn_attribs) : 
    VertexData(new VertexInternal { device, buf, fn_attribs }) { }

size_t VertexData::size()          { return m->buffer.count() / m->buffer.type_size();     }
       VertexData::operator bool() { return m->device != null; }
bool   VertexData::operator!()     { return m->device == null; }


static void glfw_error  (int code, symbol cs);
static void glfw_key    (GLFWwindow *h, int unicode, int scan_code, int action, int mods);
static void glfw_char   (GLFWwindow *h, u32 unicode);
static void glfw_button (GLFWwindow *h, int button, int action, int mods);
static void glfw_cursor (GLFWwindow *h, double x, double y);
static void glfw_resize (GLFWwindow *handle, i32 w, i32 h);

const int MAX_FRAMES_IN_FLIGHT = 2;

struct DepthTexture:Texture {
    DepthTexture(Device *device, size sz, rgba clr);
};

struct SwapImage:Texture {
    SwapImage(Device *device, size sz, VkImage vk_image);
};

struct ColorTexture:Texture {
    ColorTexture(Device *device, size sz, rgba clr);
};

struct Internal;

struct vk_interface {
    Internal *i;
    ///
    vk_interface();
    ///
    VkInstance    &instance();
    uint32_t       version ();
    Texture        texture(size sz);
};

struct Render {
    Device            *device;
    std::queue<void *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes
    array<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    array<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    array<VkFence>     fence_active;
    array<VkFence>     image_active;
    int                cframe = 0;
    int                sync = 0;
    
    inline void push(PipelineData &pd) {
        sequence.push(&pd);
    }
    ///
    Render(Device * = null);
    void present();
    void update();
};

struct window_data {
    size             sz;
    bool             headless;
    memory          *control;
    str              title;
    event            state;
    Device          *dev;
    GPU             *gpu;
    GLFWwindow      *glfw;
    VkSurfaceKHR     vk_surface; 
    Texture         *tx_skia;
    vec2             dpi_scale = { 1, 1 };
    bool             repaint;
    event            ev;
    lambda<void()>   fn_resize;
    bool             pristine;

    struct dispatchers {
        dispatch on_resize;
        dispatch on_char;
        dispatch on_button;
        dispatch on_cursor;
        dispatch on_key;
    } events;

    void button_event(states<mouse> mouse_state) {
        events.on_button(mouse_state);
    }

    void char_event(user::chr unicode) {
        events.on_char(
            event(event::edata { .chr = unicode })
        );
    }

    void wheel_event(vec2 wheel_delta) {
        events.on_cursor( wheel_delta );
    }

    void key_event  (user::key::data key) {
        events.on_cursor(
            event(event::edata { .key = key })
        );
    }

    /// app composer needs to bind to these
    void cursor_event(vec2 cursor) {
        events.on_cursor(
            event(event::edata { .cursor = cursor })
        );
    }

    void resize_event(size resize) {
        events.on_resize(
            event(event::edata { .resize = resize })
        );
    }

    ~window_data() {
        if (glfw) {
            printf("destructing window_data: %p\n", (void*)this);
            vk_interface vk;
            vkDestroySurfaceKHR(vk.instance(), vk_surface, null);
            glfwDestroyWindow(glfw);
        }
    }
};

ptr_impl(window, mx, window_data, w);

/// remove void even in lambda
/// [](args) is fine.  even (args) is fine
void window::repaint() {

}

::size window::size() { return w->sz; }

vec2 window::cursor() {
    v2<real> result { 0, 0 };
    glfwGetCursorPos(w->glfw, &result.x, &result.y);
    return vec2(result);
}

str window::clipboard() { return glfwGetClipboardString(w->glfw); }

void window::set_clipboard(str text) { glfwSetClipboardString(w->glfw, text.cs()); }

void window::set_title(str s) {
    w->title = s;
    glfwSetWindowTitle(w->glfw, w->title.cs());
}

void window::show() { glfwShowWindow(w->glfw); }
void window::hide() { glfwHideWindow(w->glfw); }

window::operator bool() { return bool(w->glfw); }

listener &dispatch::listen(callback cb) {
    listener  ls   = listener({ this, cb });
    listener &last = m.listeners.last();

    last->detatch = fn_stub([ls_mem=last.mem, listeners=&m.listeners]() -> void {
        size_t i = 0;
        bool found = false;
        for (listener &ls: *listeners) {
            if (ls.mem == ls_mem) {
                found = true;
                break;
            }
            i++;
        }
        console.test(found);
        listeners->remove(num(i)); 
    });

    return m.listeners += ls;
}

void dispatch::operator()(event e) {
    for (listener &l: m.listeners)
        l->cb(e);
}

static symbol cstr_copy(symbol s) {
    size_t len = strlen(s);
    cstr     r = (cstr)malloc(len + 1);
    std::memcpy((void *)r, (void *)s, len + 1);
    return symbol(r);
}

static void glfw_error(int code, symbol cstr) {
    console.log("glfw error: {0}", { str(cstr) });
}

/// used with terminal canvas
static map<str> t_text_colors_8 = {
    { "#000000" , "\u001b[30m" },
    { "#ff0000",  "\u001b[31m" },
    { "#00ff00",  "\u001b[32m" },
    { "#ffff00",  "\u001b[33m" },
    { "#0000ff",  "\u001b[34m" },
    { "#ff00ff",  "\u001b[35m" },
    { "#00ffff",  "\u001b[36m" },
    { "#ffffff",  "\u001b[37m" }
};

static map<str> t_bg_colors_8 = {
    { "#000000" , "\u001b[40m" },
    { "#ff0000",  "\u001b[41m" },
    { "#00ff00",  "\u001b[42m" },
    { "#ffff00",  "\u001b[43m" },
    { "#0000ff",  "\u001b[44m" },
    { "#ff00ff",  "\u001b[45m" },
    { "#00ffff",  "\u001b[46m" },
    { "#ffffff",  "\u001b[47m" }
};

/// name better, ...this.
struct vkState {
    VkPipelineVertexInputStateCreateInfo    vertex_info;
    VkPipelineInputAssemblyStateCreateInfo  topology;
    VkPipelineViewportStateCreateInfo       vs;
    VkPipelineRasterizationStateCreateInfo  rs;
    VkPipelineMultisampleStateCreateInfo    ms;
    VkPipelineDepthStencilStateCreateInfo   ds;
    VkPipelineColorBlendAttachmentState     cba;
    VkPipelineColorBlendStateCreateInfo     blending;
};

struct Descriptor {
    Device               *device;
    VkDescriptorPool      pool;
    void     destroy();
    Descriptor(Device *device = null) : device(device) { }
};

struct TextureMemory {
    Device                 *device     = null;
    VkImage                 vk_image   = VK_NULL_HANDLE;
    VkImageView             view       = VK_NULL_HANDLE;
    VkDeviceMemory          memory     = VK_NULL_HANDLE;
    VkDescriptorImageInfo   info       = {};
    VkSampler               sampler    = VK_NULL_HANDLE;
    size                    sz         = { 0, 0 };
    int                     mips       = 0;
    VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageTiling           tiling     = VK_IMAGE_TILING_OPTIMAL; /// [not likely to be a parameter]
    bool                    ms         = false;
    bool                    image_ref  = false;
    VkFormat                format;
    VkImageUsageFlags       usage;
    VkImageAspectFlags      aflags;
    image                   lazy;
    ///
    std::stack<Texture::Stage> stage;
    Texture::Stage          prv_stage       = Texture::Stage::Undefined;
    VkImageLayout           layout_override = VK_IMAGE_LAYOUT_UNDEFINED;
    ///
    void                    set_stage(Texture::Stage);
    void                    push_stage(Texture::Stage);
    void                    pop_stage();
    static int              auto_mips(uint32_t, size &);
    ///
    static VkImageView      create_view(VkDevice, size &, VkImage, VkFormat, VkImageAspectFlags, uint32_t);
    ///
    operator                bool();
    bool                    operator!();

    operator VkImage &();
    operator VkImageView &();
    operator VkDeviceMemory &();
    operator VkAttachmentDescription();
    operator VkAttachmentReference();
    operator VkDescriptorImageInfo &();
    
    VkImageView             image_view();
    VkSampler               image_sampler();
    void                    create_sampler();
    void                    create_resources();
    void                    transfer_pixels(rgba::data *pixels);

    VkWriteDescriptorSet descriptor(VkDescriptorSet *ds, uint32_t binding) {
        return VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = *ds,
            .dstBinding      = binding,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo      = &(VkDescriptorImageInfo &)*this
        };
    }
    
    TextureMemory(std::nullptr_t n = null) { }
    TextureMemory(Device *, size, rgba, VkImageUsageFlags,
            VkMemoryPropertyFlags, VkImageAspectFlags, bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1, Image = null);
    TextureMemory(Device *device,        Image &im,
            VkImageUsageFlags,     VkMemoryPropertyFlags,
            VkImageAspectFlags,    bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    TextureMemory(Device *, size, VkImage, VkImageView, VkImageUsageFlags,
            VkMemoryPropertyFlags,    VkImageAspectFlags, bool,
            VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);

    ~TextureMemory();
};

struct gpu_memory {
    struct {
        VkSurfaceCapabilitiesKHR    caps;
        array<VkSurfaceFormatKHR>   formats;
        array<VkPresentModeKHR>     present_modes;
        bool                        ansio;
        VkSampleCountFlagBits       max_sampling;
    } support;

    mx gfx, present;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    array<VkQueueFamilyProperties>  family_props;
    VkSurfaceKHR surface;

    operator VkPhysicalDevice &() { return gpu; }
    operator VkSurfaceKHR     &() { return surface; }
};

///
struct Frame {
    enum Attachment {
        Color,
        Depth,
        SwapView
    };
    ///
    int                   index;
    Device               *device;
    array<Texture>        attachments;
    VkFramebuffer         framebuffer = VK_NULL_HANDLE;
    map<VkCommandBuffer>  render_commands;
    ///
    void update();
    operator VkFramebuffer &();
    Frame(Device *device): device(device) { }
    Frame() { }
    ~Frame();
};

enums(ShadeModule, Undefined, "Undefined, Vertex, Fragment, Compute", Undefined, Vertex, Fragment, Compute);

struct Device {
    enum Module {
        Vertex,
        Fragment,
        Compute
    };
    
    struct Pair {
        Texture *texture;
        image   *img;
    };

    map<VkShaderModule> f_modules;
    map<VkShaderModule> v_modules;
    Render                render;
    VkSampleCountFlagBits sampling    = VK_SAMPLE_COUNT_8_BIT;
    VkRenderPass          render_pass = VK_NULL_HANDLE;
    Descriptor            desc;
    GPU                   gpu;
    VkCommandPool         command;
    VkDevice              device;
    VkQueue               queues[GPU::Complete];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    array<Frame>          frames;
    array<VkImage>        swap_images; // force re-render on watch event.
    Texture               tx_color;
    Texture               tx_depth;
    map<Device::Pair>     tx_cache;

    operator VkPhysicalDevice &() { return gpu; }
    operator VkCommandPool    &() { return command; }
    operator VkDevice         &() { return device; }
    operator VkRenderPass     &() { return render_pass; }
    operator VkViewport       &() { return viewport; }

    operator VkPipelineViewportStateCreateInfo() {
        sc.offset = { 0, 0 };
        sc.extent = extent;
        return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, null, 0, 1, &viewport, 1, &sc };
    }

    operator VkPipelineRasterizationStateCreateInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, null, 0, VK_FALSE, VK_FALSE,
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE // ... = 0.0f
        };
    }

    uint32_t attachment_index(TextureMemory *tx);

    void initialize(window_data *wdata);

    void update() {
        auto sz          = size { num(extent.height), num(extent.width) };
        u32  frame_count = u32(frames.len());
        render.sync++;
        /// needs a 'Device'
        tx_color = ColorTexture { this, sz, null };
        tx_depth = DepthTexture { this, sz, null };
        /// should effectively perform: ( Undefined -> Transfer, Transfer -> Shader, Shader -> Color )
        for (size_t i = 0; i < frame_count; i++) {
            Frame &frame   = frames[i];
            frame.index    = int(i);
            auto   tx_swap = SwapImage { this, sz, swap_images[i] };
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

    VkCommandBuffer begin() {
        VkCommandBuffer cb;
        VkCommandBufferAllocateInfo ai {};
        ///
        ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool        = command;
        ai.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &ai, &cb);
        ///
        VkCommandBufferBeginInfo bi {};
        bi.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);
        return cb;
    }

    void submit(VkCommandBuffer cb) {
        vkEndCommandBuffer(cb);
        ///
        VkSubmitInfo si {};
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &cb;
        ///
        VkQueue &queue          = queues[GPU::Graphics];
        vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        ///
        vkFreeCommandBuffers(device, command, 1, &cb);
    }

    void create_render_pass();

    VkQueue &operator()(GPU::Capability cap) {
        assert(cap != GPU::Complete);
        return queues[cap];
    }

    VkShaderModule module(Path, Assets &, ShadeModule);

    ~Device() {
        for (field<VkShaderModule> &field: f_modules)
            vkDestroyShaderModule(device, field.value, null);
        for (field<VkShaderModule> &field: v_modules)
            vkDestroyShaderModule(device, field.value, null);
        vkDestroyRenderPass(device, render_pass, null);
        vkDestroySwapchainKHR(device, swap_chain, null);
    }
};

Device *create_device(GPU &p_gpu, bool aa) {
    Device *dmem = new Device {
        .sampling = VK_SAMPLE_COUNT_8_BIT,
        .gpu = p_gpu
        //aa ? gpu.support.max_sampling : VK_SAMPLE_COUNT_1_BIT;
    };
    auto qcreate        = array<VkDeviceQueueCreateInfo>(2);
    float priority      = 1.0f;
    uint32_t i_gfx      = dmem->gpu.index(GPU::Graphics);
    uint32_t i_present  = dmem->gpu.index(GPU::Present);
    qcreate            += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_gfx,     1, &priority };
    if (i_present != i_gfx)
        qcreate        += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_present, 1, &priority };
    auto features       = VkPhysicalDeviceFeatures { .samplerAnisotropy = aa ? VK_TRUE : VK_FALSE };
    bool is_apple = false;
    ///
    /// silver: macros should be better, not removed. wizard once told me.
#ifdef __APPLE__
    is_apple = true;
#endif
    array<cchar_t *> ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkDeviceCreateInfo ci {};
#if !defined(NDEBUG)
    //ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME; # not supported on macOS with lunarg vk
    static cchar_t *debug   = "VK_LAYER_KHRONOS_validation";
    ci.enabledLayerCount       = 1;
    ci.ppEnabledLayerNames     = &debug;
#endif
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = uint32_t(qcreate.len());
    ci.pQueueCreateInfos       = qcreate.data();
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = uint32_t(ext.len() - size_t(!is_apple));
    ci.ppEnabledExtensionNames = ext.data();

    auto res = vkCreateDevice(dmem->gpu, &ci, nullptr, &dmem->device);
    assert(res == VK_SUCCESS);
    vkGetDeviceQueue(dmem->device, dmem->gpu.index(GPU::Graphics), 0, &dmem->queues[GPU::Graphics]); /// switch between like-queues if this is a hinderance
    vkGetDeviceQueue(dmem->device, dmem->gpu.index(GPU::Present),  0, &dmem->queues[GPU::Present]);
    /// create command pool (i think the pooling should be on each Frame)
    auto pool_info = VkCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = dmem->gpu.index(GPU::Graphics)
    };
    assert(vkCreateCommandPool(dmem->device, &pool_info, nullptr, &dmem->command) == VK_SUCCESS);
    return dmem;
}

TextureMemory::~TextureMemory() {
    if (device) {
        vkDestroyImageView(*device, view,     nullptr);
        vkDestroyImage(*device,     vk_image, nullptr);
        vkFreeMemory(*device,       memory,   nullptr);
    }
}

Frame::~Frame() {
    vkDestroyFramebuffer(*device, framebuffer, nullptr);
}

struct BufferMemory {
    Device                 *device;    operator VkDevice&()               { return *device; }
    VkDescriptorBufferInfo  info;      operator VkDescriptorBufferInfo&() { return info; }
    VkBuffer                buffer;    operator VkBuffer&()               { return buffer; }
    VkDeviceMemory          memory;    operator VkDeviceMemory&()         { return memory; }
    VkBufferUsageFlags      usage;
    VkMemoryPropertyFlags   pflags;
    size_t                  sz;
    size_t                  type_size;
    ///
    void  copy_to(BufferMemory  *, size_t);
    void  copy_to(TextureMemory *);

    BufferMemory() { }
    BufferMemory(Device *device, size_t sz, size_t type_size, void *bytes,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags pflags =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ~BufferMemory() {
        /// shouldnt be life cycle issues with this weak ref
        vkDestroyBuffer(*device, buffer, null);
        vkFreeMemory(*device, memory, null);
    }
};

struct IndexMemory {
    Device *device = null;
    Buffer  buffer;
};

ptr_impl(IndexData, mx, IndexMemory, imem);

/// implicit operator pass-through
struct UniformMemory {
    Device       *device = null;
    size_t        struct_sz;
    array<Buffer> buffers;
    UniformFn     fn;

    VkWriteDescriptorSet descriptor(size_t frame_index, VkDescriptorSet *ds);

    void transfer(size_t frame_id);
    void update(Device *dev);

    ///
    operator VkDevice  &() { return *device; }
    operator UniformFn &() { return fn; }
};

/// create with solid color
TextureMemory::TextureMemory(Device *device, size sz, rgba clr,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms,
                 VkFormat format, int mips, image lazy) :
                    device(device), sz(sz),
                    mips(auto_mips(mips, sz)), mprops(m),
                    ms(ms), format(format), usage(u), aflags(a), lazy(lazy) { }

/// create with image (rgba required)
TextureMemory::TextureMemory(Device *device, image &im,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms,
                    VkFormat           f, int  mips) :
                       device(device), sz(size { num(im.height()), num(im.width()) }), mips(auto_mips(mips, sz)),
                       mprops(m),  ms(ms),  format(f),  usage(u), aflags(a),  lazy(im) { }

TextureMemory::TextureMemory(Device *device, size sz, VkImage vk_image, VkImageView view,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms, VkFormat format, int mips):
                       device(device),  vk_image(vk_image),              view(view),
                           sz(sz),          mips(auto_mips(mips, sz)), mprops(m),
                           ms(ms),     image_ref(true),                format(format),
                        usage(u),         aflags(a) { }


void window::loop(lambda<void()> fn) {
    while (!glfwWindowShouldClose(w->glfw)) {
        if(!w->pristine) {
            fn();
            w->pristine = true;
        }
        glfwPollEvents();
    }
}

window window::handle(GLFWwindow *h) { return ((memory *)glfwGetWindowUserPointer(h))->grab(); }


VkSampleCountFlagBits max_sampling(VkPhysicalDevice gpu) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(gpu, &props);
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                props.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }
    return VK_SAMPLE_COUNT_1_BIT;
}

str terminal::ansi_color(rgba &c, bool text) {
    map<str> &map = text ? t_text_colors_8 : t_bg_colors_8;
    if (c->a < 32)
        return "";
    str hex = str("#") + str(c);
    return map.count(hex) ? map[hex] : "";
}

void terminal::draw_state_change(draw_state &ds, cbase::state_change type) {
    t.ds = &ds;
    switch (type) {
        case cbase::state_change::push:
            break;
        case cbase::state_change::pop:
            break;
        default:
            break;
    }
}

terminal::terminal(::size sz) : terminal()  {
    m.size    = sz;
    m.pixel_t = typeof(char);  
    size_t n_chars = sz.area();
    t.glyphs = array<glyph>(sz);
    for (size_t i = 0; i < n_chars; i++)
        t.glyphs += glyph {};
}

void *terminal::data() { return t.glyphs.data(); }

void terminal::set_char(int x, int y, glyph gl) {
    draw_state &ds = *t.ds;
    ::size sz = cbase::size();
    if (x < 0 || x >= sz[1]) return;
    if (y < 0 || y >= sz[0]) return;
    //assert(!ds.clip || sk_vshape_is_rect(ds.clip));
    size_t index  = y * sz[1] + x;
    t.glyphs[{y, x}] = gl;
    /*
    ----
    change-me: dont think this is required on 'set', but rather a filter on get
    ----
    if (cg.border) t.glyphs[index]->border  = cg->border;
    if (cg.chr) {
        str  gc = t.glyphs[index].chr;
        if ((gc == "+") || (gc == "|" && cg.chr == "-") ||
                            (gc == "-" && cg.chr == "|"))
            t.glyphs[index].chr = "+";
        else
            t.glyphs[index].chr = cg.chr;
    }
    if (cg.fg) t.glyphs[index].fg = cg.fg;
    if (cg.bg) t.glyphs[index].bg = cg.bg;
    */
}

// gets the effective character, including bordering
str terminal::get_char(int x, int y) {
    cbase::draw_state &ds = *t.ds;
    ::size        &sz = m.size;
    size_t       ix = math::clamp<size_t>(x, 0, sz[1] - 1);
    size_t       iy = math::clamp<size_t>(y, 0, sz[0] - 1);
    str       blank = " ";
    
    auto get_str = [&]() -> str {
        auto value_at = [&](int x, int y) -> glyph * {
            if (x < 0 || x >= sz[1]) return null;
            if (y < 0 || y >= sz[0]) return null;
            return &t.glyphs[y * sz[1] + x];
        };
        
        auto is_border = [&](int x, int y) {
            glyph *cg = value_at(x, y);
            return cg ? ((*cg)->border > 0) : false;
        };
        
        glyph::members &cg = t.glyphs[iy * sz[1] + ix];
        auto   t  = is_border(x, y - 1);
        auto   b  = is_border(x, y + 1);
        auto   l  = is_border(x - 1, y);
        auto   r  = is_border(x + 1, y);
        auto   q  = is_border(x, y);
        
        if (q) {
            if (t && b && l && r) return "\u254B"; //  +
            if (t && b && r)      return "\u2523"; //  |-
            if (t && b && l)      return "\u252B"; // -|
            if (l && r && t)      return "\u253B"; // _|_
            if (l && r && b)      return "\u2533"; //  T
            if (l && t)           return "\u251B";
            if (r && t)           return "\u2517";
            if (l && b)           return "\u2513";
            if (r && b)           return "\u250F";
            if (t && b)           return "\u2503";
            if (l || r)           return "\u2501";
        }
        return cg.chr;
    };
    return get_str();
}

void terminal::text(str s, graphics::shape vrect, alignment::data &align, vec2 voffset, bool ellip) {
    r4<real>    rect   =  vrect.bounds();
    ::size       &sz   =  cbase::size();
    v2<real>   &offset =  voffset;
    draw_state &ds     = *t.ds;
    int         len    =  int(s.len());
    int         szx    = sz[1];
    int         szy    = sz[0];
    ///
    if (len == 0)
        return;
    ///
    int x0 = math::clamp(int(math::round(rect.x)), 0,          szx - 1);
    int x1 = math::clamp(int(math::round(rect.x + rect.w)), 0, szx - 1);
    int y0 = math::clamp(int(math::round(rect.y)), 0,          szy - 1);
    int y1 = math::clamp(int(math::round(rect.y + rect.h)), 0, szy - 1);
    int  w = (x1 - x0) + 1;
    int  h = (y1 - y0) + 1;
    ///
    if (!w || !h) return;
    ///
    int tx = align.x == xalign::left   ? (x0) :
                align.x == xalign::middle ? (x0 + (x1 - x0) / 2) - int(std::ceil(len / 2.0)) :
                align.x == xalign::right  ? (x1 - len) : (x0);
    ///
    int ty = align.y == yalign::top    ? (y0) :
                align.y == yalign::middle ? (y0 + (y1 - y0) / 2) - int(std::ceil(len / 2.0)) :
                align.y == yalign::bottom ? (y1 - len) : (y0);
    ///
    tx           = math::clamp(tx, x0, x1);
    ty           = math::clamp(ty, y0, y1);
    size_t ix    = math::clamp(size_t(tx), size_t(0), size_t(szx - 1));
    size_t iy    = math::clamp(size_t(ty), size_t(0), size_t(szy - 1));
    size_t len_w = math::  min(size_t(x1 - tx), size_t(len));
    ///
    for (size_t i = 0; i < len_w; i++) {
        int x = ix + i + offset.x;
        int y = iy + 0 + offset.y;
        if (x < 0 || x >= szx || y < 0 || y >= szy)
            continue;
        str     chr  = s.mid(i, 1);
        set_char(tx + int(i), ty,
            glyph::members {
                0, chr, {0,0,0,0}, rgba { ds.color }
            });
    }
}

void terminal::outline(graphics::shape sh) {
    draw_state &ds = *t.ds;
    assert(sh);
    r4r     &r = sh.bounds();
    int     ss = math::min(2.0, math::round(ds.outline_sz));
    c4<u8>  &c = ds.color;
    glyph cg_0 = glyph::members { ss, "-", null, rgba { c.r, c.g, c.b, c.a }};
    glyph cg_1 = glyph::members { ss, "|", null, rgba { c.r, c.g, c.b, c.a }};
    
    for (int ix = 0; ix < int(r.w); ix++) {
        set_char(int(r.x + ix), int(r.y), cg_0);
        if (r.h > 1)
            set_char(int(r.x + ix), int(r.y + r.h - 1), cg_0);
    }
    for (int iy = 0; iy < int(r.h); iy++) {
        set_char(int(r.x), int(r.y + iy), cg_1);
        if (r.w > 1)
            set_char(int(r.x + r.w - 1), int(r.y + iy), cg_1);
    }
}

void terminal::fill(graphics::shape sh) {
    draw_state &ds = *t.ds;
    ::size       &sz = cbase::size();
    int         sx = sz[1];
    int         sy = sz[0];
    if (ds.color) {
        r4r &r       = sh.rect();
        str  t_color = ansi_color(ds.color, false);
        ///
        int      x0 = math::max(int(0),        int(r.x)),
                    x1 = math::min(int(sx - 1),   int(r.x + r.w));
        int      y0 = math::max(int(0),        int(r.y)),
                    y1 = math::min(int(sy - 1),   int(r.y + r.h));

        glyph    cg = glyph::members { 0, " ", rgba(ds.color), null }; /// members = data

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                set_char(x, y, cg); /// just set mem w grab()
    }
}

/// allocate memory with reference space (recycleable) 
/// and use the external attachment so the generics do not allocate or delete pointer
#define palloc(MX, RES, PTR, ...)\
do {\
    static MX*abc_test = nullptr;\
    static bool proceed = identical<typename MX::MEM, mem_ptr_token>();\
    assert(proceed);\
    memory         *_mem = memory::alloc(typeof(MX));\
    typename MX::DC *_tm = new typename MX::DC(__VA_ARGS__);\
    _mem->origin         = (void*)_tm;\
    _mem->attach("intern", _tm, [_tm]() -> void { delete _tm; });\
    RES = _mem;\
    PTR = _tm;\
} while (0);

/// the pointer assignment is additional validation between DC and user-declared var type

struct memory;
/// deprecate all other uses of Texture() construction
/// internals are nice to design into the module, and thus the types can be looked up in .cpp
/// or like something like 'defer'
struct Texture;
static Texture create_texture(Device *device,  size sz, rgba clr,
        VkImageUsageFlags  vk_usage,   VkMemoryPropertyFlags vk_mem,
        VkImageAspectFlags vk_aspect,  bool                  ms,
        VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1, image lazy = null)
{
    memory        *mem; // resulting memory* handle
    TextureMemory *tm;  // resulting TextureMemory (allocated in-module)
    /// important to give the context type; there should always be access to that
    palloc(Texture, mem, tm,
        device,   sz,      clr,
        vk_usage, vk_mem,  vk_aspect, ms,
        format,   mips,    lazy);
    
    ///
    rgba::data *px = null;
    if (clr) {
        size_t area = sz.area();
        px = (rgba::data *)malloc(area * sizeof(rgba::data));
        for (int i = 0; i < sz[0] * sz[1]; i++)
            px[i] = clr;
    }
    tm->transfer_pixels(px);
    free(px);
    ///
    return Texture(mem);
}

/// converting the constructors to Texture
static Texture create_texture(Device *device, image &im,
        VkImageUsageFlags  usage,  VkMemoryPropertyFlags vk_mem,
        VkImageAspectFlags aspect, bool                  ms,
        VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB,
        int                mips   = -1) {
    memory        *mem;
    TextureMemory *data; 
    palloc(Texture, mem, data,
        device, im, usage, vk_mem, aspect, ms, format, mips);

    ///
    rgba::data *px = &im[{0,0}];
    data->transfer_pixels(px);
    return Texture(mem);
}

static Texture create_texture(Device *device, size sz,
        VkImage         vk_image,  VkImageView           view,
        VkImageUsageFlags  usage,  VkMemoryPropertyFlags vk_mem,
        VkImageAspectFlags aspect, bool                  ms,
        VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1) {

    memory        *mem;
    TextureMemory *data; 
    palloc(Texture, mem, data,
        device, sz, vk_image, view, usage, vk_mem, aspect, ms, format, mips);
    return Texture(mem);
}

Texture::Stage::Stage(Stage::Type value) : value(value) { }

struct StageData {
    enum Texture::Stage::Type value  = Texture::Stage::Undefined;
    VkImageLayout           layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint64_t                access = 0;
    VkPipelineStageFlagBits stage;
    inline bool operator==(StageData &b) { return layout == b.layout; }
};

static const StageData stage_types[5] = {
    { Texture::Stage::Undefined, VK_IMAGE_LAYOUT_UNDEFINED,                0,                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { Texture::Stage::Transfer,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
    { Texture::Stage::Shader,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  },
    { Texture::Stage::Color,     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { Texture::Stage::Depth,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
};

/// likely does not need to remain exposed as texture api; i just want these to be file-derived for all params, just from its name alone.
const StageData *Texture::Stage::data() { return &stage_types[value]; }

void TextureMemory::create_sampler() {
    VkPhysicalDeviceProperties props { };
    vkGetPhysicalDeviceProperties(*device, &props);
    ///
    VkSamplerCreateInfo si { };
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
    VkResult r = vkCreateSampler(*device, &si, nullptr, &sampler);
    assert  (r == VK_SUCCESS);
}

void     Texture::set_stage (Stage s) { return data->set_stage(s);  }
void     Texture::push_stage(Stage s) { return data->push_stage(s); }
void     Texture::pop_stage()         { return data->pop_stage();   }

uint32_t memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props) {
   VkPhysicalDeviceMemoryProperties mprops;
   vkGetPhysicalDeviceMemoryProperties(gpu, &mprops);
   ///
   for (uint32_t i = 0; i < mprops.memoryTypeCount; i++)
       if ((types & (1 << i)) && (mprops.memoryTypes[i].propertyFlags & props) == props)
           return i;
   /// no flags match
   assert(false);
   return 0;
};

void TextureMemory::create_resources() {
    VkDevice           vk_dev   = device->device;
    VkPhysicalDevice   vk_gpu   = device->gpu;
    VkSampleCountFlags sampling = device->sampling;
    VkImageCreateInfo  imi { };
    ///
    layout_override             = VK_IMAGE_LAYOUT_UNDEFINED;
    ///
    imi.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imi.imageType               = VK_IMAGE_TYPE_2D;
    imi.extent.width            = u32(sz[1]);
    imi.extent.height           = u32(sz[0]);
    imi.extent.depth            = 1;
    imi.mipLevels               = mips;
    imi.arrayLayers             = 1;
    imi.format                  = format;
    imi.tiling                  = tiling;
    imi.initialLayout           = layout_override;
    imi.usage                   = usage;
    imi.samples                 = ms ? device->sampling : VK_SAMPLE_COUNT_1_BIT;
    imi.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    ///
    assert(vkCreateImage(vk_dev, &imi, nullptr, &vk_image) == VK_SUCCESS);
    ///
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vk_dev, vk_image, &req);
    VkMemoryAllocateInfo a {};
    a.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    a.allocationSize            = req.size;
    a.memoryTypeIndex           = memory_type(vk_gpu, req.memoryTypeBits, mprops); /// this should work; they all use the same mprops from what i've seen so far and it stores that default
    assert(vkAllocateMemory(vk_dev, &a, nullptr, &memory) == VK_SUCCESS);
    vkBindImageMemory(vk_dev, vk_image, memory, 0);
    ///
    if (ms) create_sampler();
    ///
    // run once like this:
    //set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); /// why?
}

void TextureMemory::transfer_pixels(rgba::data *pixels) {
    create_resources(); // usage not copied?
    if (pixels) {
        size_t nbytes = VkDeviceSize(sz.area() * 4); /// adjust bytes if format isnt rgba; implement grayscale
        Buffer    src = create_buffer(device, sz.area(), typeof(u32), null, Buffer::Usage::Src);
        void* raw_mem = null;
        vkMapMemory(*device, *src.bmem, 0, nbytes, 0, &raw_mem);
        memcpy(raw_mem, pixels, size_t(nbytes));
        vkUnmapMemory(*device, *src.bmem);
        ///
        push_stage(Texture::Stage::Transfer);
        src->copy_to(this);
      //pop_stage(); // ???
    }
    image_ref = false;
}

void TextureMemory::pop_stage() {
    assert(stage.size() >= 2);
    /// kind of need to update it.
    stage.pop();
    set_stage(stage.top());
}

void TextureMemory::set_stage(Texture::Stage next_stage) {
    Texture::Stage cur  = prv_stage;
    while (cur != next_stage) {
        Texture::Stage  from  = cur;
        Texture::Stage  to    = cur;
        ///
        /// skip over that color thing because this is quirky
        do to = Texture::Stage::Type(int(to) + ((next_stage > prv_stage) ? 1 : -1));
        while (to == Texture::Stage::Color && to != next_stage);
        ///
        /// transition through the image membrane, as if we were insane in-the
        const StageData      *data = from.data();
        const StageData *next_data =   to.data();
        VkImageMemoryBarrier   barrier = {
            .sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask        = VkAccessFlags(     data->access),
            .dstAccessMask        = VkAccessFlags(next_data->access),
            .oldLayout            =      data->layout,
            .newLayout            = next_data->layout,
            .srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED,
            .image                = vk_image,
            .subresourceRange     = {
                .aspectMask       = aflags,
                .baseMipLevel     = 0,
                .levelCount       = uint32_t(mips),
                .baseArrayLayer   = 0,
                .layerCount       = 1
            }
        };
        VkCommandBuffer cb = device->begin();
        vkCmdPipelineBarrier(
            cb, data->stage, next_data->stage, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);
        device->submit(cb);
        cur = to;
    }
    prv_stage = next_stage;
}

void TextureMemory::push_stage(Texture::Stage next_stage) {
    if (stage.size() == 0)
        stage.push(Texture::Stage::Undefined); /// default stage, when pop'd
    if (next_stage == stage.top())
        return;
    
    set_stage(next_stage);
    stage.push(next_stage);
}

VkImageView TextureMemory::create_view(VkDevice device, size &sz, VkImage vk_image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageView   view;
    uint32_t      mips = auto_mips(mip_levels, sz);
  //auto         usage = VkImageViewUsageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, null, VK_IMAGE_USAGE_SAMPLED_BIT };
    auto             v = VkImageViewCreateInfo {};
    v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  //v.pNext            = &usage;
    v.image            = vk_image;
    v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    v.format           = format;
    v.subresourceRange = { aspect_flags, 0, mips, 0, 1 };
    VkResult res = vkCreateImageView(device, &v, nullptr, &view);
    assert (res == VK_SUCCESS);
    return view;
}

int TextureMemory::auto_mips(uint32_t mips, size &sz) {
    return 1; /// mips == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : std::max(1, mips);
}

TextureMemory::operator bool () { return vk_image != VK_NULL_HANDLE; }
bool TextureMemory::operator!() { return vk_image == VK_NULL_HANDLE; }

TextureMemory::operator VkImage &() {
    return vk_image;
}

TextureMemory::operator VkImageView &() {
    if (!view)
         view = create_view(*device, sz, vk_image, format, aflags, mips); /// hide away the views, only create when they are used
    return view;
}

TextureMemory::operator VkDeviceMemory &() {
    return memory;
}

TextureMemory::operator VkAttachmentDescription() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto     desc = VkAttachmentDescription {
        .format         = format,
        .samples        = ms ? device->sampling : VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = image_ref ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                      VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        =  is_color ? VK_ATTACHMENT_STORE_OP_STORE :
                                      VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        /// odd thing to need to pass the life cycle like this
        .finalLayout    = image_ref  ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                           is_color  ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    return desc;
}

TextureMemory::operator VkAttachmentReference() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  //bool  is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t index = device->attachment_index(this);
    return VkAttachmentReference {
        .attachment  = index,
        .layout      = (layout_override != VK_IMAGE_LAYOUT_UNDEFINED) ? layout_override :
                       (stage.size() ? stage.top().data()->layout : VK_IMAGE_LAYOUT_UNDEFINED)
    };
}

VkImageView TextureMemory::image_view() {
    if (!view)
         view = create_view(*device, sz, vk_image, format, aflags, mips);
    return view;
}

VkSampler TextureMemory::image_sampler() {
    if (!sampler) create_sampler();
    return sampler;
}

TextureMemory::operator VkDescriptorImageInfo &() {
    info  = {
        .sampler     = image_sampler(),
        .imageView   = image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    return info;
}

DepthTexture::DepthTexture(Device *device, size sz, rgba clr):
    Texture(create_texture(device, sz, clr,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT, true, VK_FORMAT_D32_SFLOAT, 1)) {
        data->image_ref = false; /// we are explicit here.. just plain explicit.
        data->push_stage(Stage::Depth);
    }
    
ColorTexture::ColorTexture(Device *device, size sz, rgba clr):
    Texture(create_texture(device, sz, clr,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_UNORM, 1)) { // was VK_FORMAT_B8G8R8A8_SRGB
        data->image_ref = false; /// 'SwapImage' should have this set as its a View-only.. [/creeps away slowly, very unsure of themselves]
        data->push_stage(Stage::Color);
    }

SwapImage::SwapImage(Device *device, size sz, VkImage vk_image):
    Texture(create_texture(device, sz, vk_image, null,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_B8G8R8A8_UNORM, 1)) { // VK_FORMAT_B8G8R8A8_UNORM tried here, no effect on SwapImage
        data->layout_override = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

/// could be called surface binding i suppose thats a bit more to its application
GPU create_gpu(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    memory      *mem; // resulting memory* handle
    gpu_memory  *mgpu;  // resulting TextureMemory (allocated in-module)
    /// important to give the context type; there should always be access to that
    palloc(GPU, mem, mgpu);
    
    /// set fields
    mgpu->gpu       = gpu;
    mgpu->surface   = surface;
    uint32_t fcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, nullptr);
    mgpu->family_props.set_size(fcount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, mgpu->family_props.data());
    uint32_t index = 0;
    /// we need to know the indices of these
    mgpu->gfx     = mx(false);
    mgpu->present = mx(false);
    
    for (const auto &fprop: mgpu->family_props) { // check to see if it steps through data here.. thats the question
        VkBool32 has_present;
        vkGetPhysicalDeviceSurfaceSupportKHR(mgpu->gpu, index, mgpu->surface, &has_present);
        bool has_gfx = (fprop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        ///
        /// prefer to get gfx and present in the same family_props
        if (has_gfx) {
            mgpu->gfx = mx(index);
            /// query surface formats
            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, mgpu->surface, &format_count, nullptr);
            if (format_count != 0) {
                mgpu->support.formats.set_size(format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                    mgpu->gpu, surface, &format_count, mgpu->support.formats.data());
            }
            /// query swap chain support, store on GPU
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                mgpu->gpu, mgpu->surface, &mgpu->support.caps);
            /// ansio relates to gfx
            VkPhysicalDeviceFeatures fx;
            vkGetPhysicalDeviceFeatures(gpu, &fx);
            mgpu->support.ansio = fx.samplerAnisotropy;
        }
        ///
        if (bool(has_present)) {
            mgpu->present = var(index);
            uint32_t count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                mgpu->gpu, mgpu->surface, &count, nullptr);
            if (count != 0) {
                mgpu->support.present_modes.set_size(count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                    mgpu->gpu, mgpu->surface, &count, mgpu->support.present_modes);
            }
        }
        if (bool(has_gfx) & bool(has_present))
            break;
        index++;
    }
    if (mgpu->support.ansio)
        mgpu->support.max_sampling = max_sampling(mgpu->gpu);
    return GPU(mem);
}

void UniformMemory::transfer(size_t frame_id) {
    Buffer  &buffer = buffers[frame_id]; /// todo -- UniformData never initialized (problem, that is)
    size_t   sz     = buffer->sz;
    cchar_t *data[2048]; /// needs to be sized according to some large number you keep upping when you make different gates-like statements

    /// call lambda for uniform control and copy memory to gpu
    void*    v_gpu;
    memset  (data, 0, sizeof(data));
    fn(data);
    vkMapMemory(*device, buffer, VkDeviceSize(0), VkDeviceSize(sz), VkMemoryMapFlags(0), (void**)&v_gpu);
    memcpy(v_gpu, data, sz);
    
    /// 
    vkUnmapMemory(*device, buffer);
}


/// obtain write desciptor set
VkWriteDescriptorSet UniformMemory::descriptor(size_t frame_index, VkDescriptorSet *ds) {
    return {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, null, *ds, 0, 0,
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, null, &buffers[frame_index]->info, null
    };
}

/// lazy initialization of Uniforms
void UniformMemory::update(Device *dev) {
    //for (auto &b: m.buffers)
    //    b.destroy();
    device = dev;
    size_t n_frames = device->frames.len();
    buffers = array<Buffer>(n_frames);
    ///
    for (int i = 0; i < n_frames; i++)
        buffers += new BufferMemory {
            device, 1, size_t(struct_sz), (void*)null, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        };
}

VkShaderModule Device::module(Path path, Assets &assets, ShadeModule type) {
    str    key = path;
    auto &modules = type == ShadeModule::Vertex ? v_modules : f_modules;
    Path   spv = fmt {"{0}.spv", { key }};
    
#if !defined(NDEBUG)
    if (!spv.exists() || path.modified_at() > spv.modified_at()) {
        if (modules.count(key))
            modules.remove(key);
        str   defs = "";
        auto remap = map<str> {
            { Asset::Color,    " -DCOLOR"    },
            { Asset::Specular, " -DSPECULAR" },
            { Asset::Displace, " -DDISPLACE" },
            { Asset::Normal,   " -DNORMAL"   }
        };
        
        for (auto &[type, tx]: assets) /// always use type when you can, over id (id is genuinely instanced things)
            defs += remap[type];
        
        /// path was /usr/local/bin/ ; it should just be in path, and to support native windows
        Path command = fmt {"glslc {1} {0} -o {0}.spv", { key, defs }};
        async { command }.sync();
        
        ///
        Path tmp = "./.tmp/"; ///
        if (spv.exists() && spv.modified_at() >= path.modified_at()) {
            spv.copy(tmp);
        } else {
            // look for tmp. if this does not exist we can not proceed.
            // compilation failure, so look for temp which worked before.
        }
        
        /// if it succeeds, the spv is written and that will have a greater modified-at
    }
#endif

    if (!modules.count(key)) {
        auto mc     = VkShaderModuleCreateInfo { };
        str code    = str::read_file(spv);
        mc.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        mc.codeSize = code.len();
        mc.pCode    = reinterpret_cast<const uint32_t *>(code.cs());
        assert (vkCreateShaderModule(device, &mc, null, &modules[key]) == VK_SUCCESS);
    }
    return modules[key];
}

ptr_impl(Buffer,       mx, BufferMemory,   bmem);
ptr_impl(GPU,          mx, gpu_memory,     gmem);
ptr_impl(Texture,      mx, TextureMemory,  data);
ptr_impl(PipelineData, mx, PipelineMemory, m);
ptr_impl(UniformData,  mx, UniformMemory,  umem);
ptr_impl(VertexData,   mx, VertexInternal, m);

/// this is to avoid doing explicit monkey-work, and keep code size down as well as find misbound texture
uint32_t Device::attachment_index(TextureMemory *tx) {
    auto is_referencing = [&](TextureMemory *a_data, Texture *b) {
        if (!a_data && !b->data)
            return false;
        if (a_data && b->data && a_data->vk_image && a_data->vk_image == b->data->vk_image)
            return true;
        if (a_data && b->data && a_data->view  && a_data->view  == b->data->view)
            return true;
        return false;
    };
    for (auto &f: frames)
        for (int i = 0; i < f.attachments.len(); i++)
            if (is_referencing(tx, &f.attachments[i]))
                return i;
    assert(false);
    return 0;
}

void Device::initialize(window_data *wdata) {
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
    auto swap_extent = [pw=wdata](VkSurfaceCapabilitiesKHR& caps) -> VkExtent2D {
        if (caps.currentExtent.width != UINT32_MAX) {
            return caps.currentExtent;
        } else {
            int w, h;
            glfwGetFramebufferSize(pw->glfw, &w, &h);
            VkExtent2D ext = { uint32_t(w), uint32_t(h) };
            ext.width      = std::clamp(ext.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
            ext.height     = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            return ext;
        }
    };
    ///
    auto    &support              = gpu->support;
    uint32_t gfx_index            = gpu.index(GPU::Graphics);
    uint32_t present_index        = gpu.index(GPU::Present);
    auto     surface_format       = select_surface_format(support.formats);
    auto     surface_present      = select_present_mode(support.present_modes);
    auto     extent               = swap_extent(support.caps);
    uint32_t frame_count          = support.caps.minImageCount + 1;
    uint32_t indices[]            = { gfx_index, present_index };
    ///
    if (support.caps.maxImageCount > 0 && frame_count > support.caps.maxImageCount)
        frame_count               = support.caps.maxImageCount;
    ///
    VkSwapchainCreateInfoKHR ci { };
    ci.sType                      = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface                    = wdata->vk_surface;
    ci.minImageCount              = frame_count;
    ci.imageFormat                = surface_format.format;
    ci.imageColorSpace            = surface_format.colorSpace;
    ci.imageExtent                = extent;
    ci.imageArrayLayers           = 1;
    ci.imageUsage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ///
    if (gfx_index != present_index) {
        ci.imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount  = 2;
        ci.pQueueFamilyIndices    = indices;
    } else
        ci.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    ///
    ci.preTransform               = support.caps.currentTransform;
    ci.compositeAlpha             = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode                = surface_present;
    ci.clipped                    = VK_TRUE;
    
    /// create swap-chain
    assert(vkCreateSwapchainKHR(device, &ci, nullptr, &swap_chain) == VK_SUCCESS);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, nullptr); /// the dreaded frame_count (2) vs image_count (3) is real.
    frames = array<Frame>(size_t(frame_count), Frame(this));
    
    /// get swap-chain images
    swap_images.set_size(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format             = surface_format.format;
    this->extent       = extent;
    viewport           = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    const int guess    = 8;
    auto      ps       = std::array<VkDescriptorPoolSize, 3> {};
    ps[0]              = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         frame_count * guess };
    ps[1]              = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count * guess };
    ps[2]              = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count * guess };
    auto dpi           = VkDescriptorPoolCreateInfo {
                            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                            frame_count * guess, uint32_t(frame_count), ps.data() };
    render             = Render(this);
    desc               = Descriptor(this);
    ///
    assert(vkCreateDescriptorPool(device, &dpi, nullptr, &desc.pool) == VK_SUCCESS);
    for (auto &f: frames)
        f.index        = -1;
    update();
}


#if 0
VkFormat supported_format(VkPhysicalDevice gpu, array<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &props);

        bool fmatch = (props.linearTilingFeatures & features) == features;
        bool tmatch = (tiling == VK_IMAGE_TILING_LINEAR || tiling == VK_IMAGE_TILING_OPTIMAL);
        if  (fmatch && tmatch)
            return format;
    }

    throw std::runtime_error("failed to find supported format!");
}

static VkFormat get_depth_format(VkPhysicalDevice gpu) {
    return supported_format(gpu,
        {VK_FORMAT_D32_SFLOAT,    VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
         VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
};
#endif


void Device::create_render_pass() {
    Frame &f0                     = frames[0];
    Texture &tx_ref               = f0.attachments[Frame::SwapView]; /// has msaa set to 1bit
    assert(f0.attachments.len() == 3);
    VkAttachmentReference    cref = VkAttachmentReference(tx_color); // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    dref = VkAttachmentReference(tx_depth); // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    rref = VkAttachmentReference(tx_ref);   // the odd thing is the associated COLOR_ATTACHMENT layout here;
    VkSubpassDescription     sp   = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, null, 1, &cref, &rref, &dref };

    VkSubpassDependency dep { };
    dep.srcSubpass                = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass                = 0;
    dep.srcStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask             = 0;
    dep.dstStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask             = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription adesc_clr_image  = *f0.attachments[0].data;
    VkAttachmentDescription adesc_dep_image  = *f0.attachments[1].data; // format is wrong here, it should be the format of the tx
    VkAttachmentDescription adesc_swap_image = *f0.attachments[2].data;
    
    std::array<VkAttachmentDescription, 3> attachments = {adesc_clr_image, adesc_dep_image, adesc_swap_image};
    VkRenderPassCreateInfo rpi { }; // VkAttachmentDescription needs to be 4, 4, 1
    rpi.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount           = uint32_t(attachments.size());
    rpi.pAttachments              = attachments.data();
    rpi.subpassCount              = 1;
    rpi.pSubpasses                = &sp;
    rpi.dependencyCount           = 1;
    rpi.pDependencies             = &dep;
    ///
    /// seems like tx_depth is not created with a 'depth' format, or improperly being associated to a color format
    ///
    assert(vkCreateRenderPass(device, &rpi, nullptr, &render_pass) == VK_SUCCESS);
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


///
void Frame::update() {
    vkDestroyFramebuffer(*device, framebuffer, nullptr);
    auto views = array<VkImageView>();
    for (Texture &tx: attachments) {
        VkImageView iv = (VkImageView&)tx; /// trace this one make sure it goes into the VkImageView& operator
        views += iv;
    }
    VkFramebufferCreateInfo ci { };
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = device->render_pass;
    ci.attachmentCount = u32(views.len());
    ci.pAttachments    = views.data();
    ci.width           = device->extent.width;
    ci.height          = device->extent.height;
    ci.layers          = 1;
    assert(vkCreateFramebuffer(*device, &ci, nullptr, &framebuffer) == VK_SUCCESS);
}

Frame::operator VkFramebuffer &() {
    return framebuffer;
}

/// isolate pipeline memory
struct PipelineMemory {
    Device                    *device;
    str                        shader;
    array<VkCommandBuffer>     frame_commands;   /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
    array<VkDescriptorSet>     desc_sets;        /// needs to be in frame, i think.
    VkPipelineLayout           pipeline_layout;  /// and what a layout
    VkPipeline                 pipeline;         /// pipeline handle
    UniformData                ubo;              /// we must broadcast this buffer across to all of the swap uniforms
    VertexData                 vbo;              ///
    IndexData                  ibo;
    // array<VkVertexInputAttributeDescription> attr; -- vector is given via VertexData::fn_attribs(). i dont believe those need to be pushed up the call chain
    Assets                     assets;
    VkDescriptorSetLayout      set_layout;
    bool                       enabled = true;
    map<Texture *>             tx_cache;
    size_t                     vsize;
    rgba                       clr;
    ///
    /// state vars
    int                        sync = -1;
    ///
    operator bool ();
    bool operator!();
    void enable (bool en);
    void update(size_t frame_id);
    PipelineMemory(std::nullptr_t    n = null);
    PipelineMemory(Device *device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
            Assets &assets, size_t vsize, rgba clr, str name, VkStateFn vk_state);
    
    ~PipelineMemory() {
        auto  &device = *this->device;
        vkDestroyDescriptorSetLayout(device, set_layout, null);
        vkDestroyPipeline(device, pipeline, null);
        vkDestroyPipelineLayout(device, pipeline_layout, null);
        for (auto &cmd: frame_commands)
            vkFreeCommandBuffers(device, device.command, 1, &cmd);
    }

    void initialize();
};

PipelineData::PipelineData(Device *device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                Assets &assets, size_t vsize, rgba clr, str shader, VkStateFn vk_state) {
    m = new PipelineMemory { device, ubo, vbo, ibo, assets, vsize, clr, shader, vk_state };
}


       PipelineData::operator bool()                { return  m->enabled; }
bool   PipelineData::operator!    ()                { return !m->enabled; }
void   PipelineData::enable       (bool en)         { m->enabled = en; }
bool   PipelineData::operator==   (PipelineData &b) { return m == b.m; }
void   PipelineData::update       (size_t frame_id) {
    m->update(frame_id);
}


Pipes::Pipes(Device *device) : mx(alloc<Pipes>()), d(ref<Pipes>()) {
    d.device = device;
}

///
PipelineMemory::PipelineMemory(std::nullptr_t n) { }

/// constructor for pipeline memory; worth saying again.
PipelineMemory::PipelineMemory(Device   *device,  UniformData &ubo,
                             VertexData    &vbo,     IndexData   &ibo,
                             Assets     &assets,     size_t       vsize, // replace all instances of array<Texture *> with a map<str, Teture *>
                             rgba           clr,     str       shader,
                             VkStateFn      vk_state):
        device(device),  shader(shader),   ubo(ubo),
           vbo(vbo),        ibo(ibo),   assets(assets),  vsize(vsize),   clr(clr) {
    ///
    auto bindings  = array<VkDescriptorSetLayoutBinding>(1 + assets.count());
         bindings += { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
    
    
    /// binding ids for resources are reflected in shader
    /// todo: write big test for this
    for (field<Texture*> &f:assets) {
        Asset  a  = f.key.type() == typeof(int) ? int(f.key) : f.key;
        bindings += Asset_descriptor(a); // likely needs tx in arg form in this generic
    }

    /// create descriptor set layout
    auto desc_layout_info = VkDescriptorSetLayoutCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0, uint32_t(bindings.len()), bindings.data() }; /// todo: fix general pipline. it will be different for Compute Pipeline, but this is reasonable for now if a bit lazy
    VkResult vk_desc_res = vkCreateDescriptorSetLayout(*device, &desc_layout_info, null, &set_layout);
    console.test(vk_desc_res == VK_SUCCESS, "vkCreateDescriptorSetLayout failure with {0}", { vk_desc_res });
    
    /// of course, you do something else right about here...
    const size_t  n_frames = device->frames.len();
    auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, set_layout); /// add to vec.
    auto                ai = VkDescriptorSetAllocateInfo {};
    ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool      = device->desc.pool;
    ai.descriptorSetCount  = u32(n_frames);
    ai.pSetLayouts         = layouts.data();
    desc_sets.set_size(n_frames);
    VkDescriptorSet *ds_data = desc_sets.data();
    VkResult res = vkAllocateDescriptorSets(*device, &ai, ds_data);
    assert  (res == VK_SUCCESS);
    ubo->update(device);
    ///
    /// write descriptor sets for all swap image instances
    for (size_t i = 0; i < device->frames.len(); i++) {
        VkDescriptorSet &desc_set = desc_sets[i]; /// makes zero sense here. this is the data. i dunno what this macro is in vulkan for the *_T but its distractive lol
        array<VkWriteDescriptorSet> v_writes(size_t(1 + assets.count()));
        
        /// update descriptor sets
        VkWriteDescriptorSet wds = ubo->descriptor(i, &desc_set);
        v_writes       += wds;
        for (field<Texture*> &f:assets) {
            Asset    a  = f.key;
            Texture *tx = f.value;
            v_writes += tx->data->descriptor(&desc_set, uint32_t(int(a)));
        }
        
        VkDevice    dev = *device;
        size_t       sz = v_writes.len();
        VkWriteDescriptorSet *ptr = v_writes.data();
        vkUpdateDescriptorSets(dev, uint32_t(sz), ptr, 0, nullptr);
    }
    str    cwd = Path::cwd();
    str s_vert = str::format("{0}/shaders/{1}.vert", { cwd, shader });
    str s_frag = str::format("{0}/shaders/{1}.frag", { cwd, shader });
    auto  vert = device->module(s_vert, assets, ShadeModule::Vertex);
    auto  frag = device->module(s_frag, assets, ShadeModule::Fragment);
    ///
    array<VkPipelineShaderStageCreateInfo> stages {{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_VERTEX_BIT,   vert, shader.cs(), null
    }, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_FRAGMENT_BIT, frag, shader.cs(), null
    }};
    ///
    auto binding            = VkVertexInputBindingDescription { 0, uint32_t(vsize), VK_VERTEX_INPUT_RATE_VERTEX };
    
    /// 
    VkAttribs vk_attr;
    vbo->fn_attribs(&vk_attr);
    
    auto viewport           = VkViewport(*device);
    auto sc                 = VkRect2D {
        .offset             = {0, 0},
        .extent             = device->extent
    };
    auto cba                = VkPipelineColorBlendAttachmentState {
        .blendEnable        = VK_FALSE,
        .colorWriteMask     = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    auto layout_info        = VkPipelineLayoutCreateInfo {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount     = 1,
        .pSetLayouts        = &set_layout
    };
    ///
    assert(vkCreatePipelineLayout(*device, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);

    /*
    VkStructureType   sType;
    const  void *             pNext;
    VkPipelineVertexInputStateCreateFlags      flags;
    uint32_t                 vertexBindingDescriptionCount;
    const  VkVertexInputBindingDescription *  pVertexBindingDescriptions;
    uint32_t                 vertexAttributeDescriptionCount;
    const  VkVertexInputAttributeDescription *  pVertexAttributeDescriptions;
    */
    /// ok initializers here we go.
    vkState state {
        .vertex_info = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (const VkVertexInputBindingDescription *)&binding,
            .vertexAttributeDescriptionCount = uint32_t(vk_attr.len()),
            .pVertexAttributeDescriptions    = (const VkVertexInputAttributeDescription *)vk_attr.data()
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
            .rasterizationSamples    = device->sampling,
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
        .stageCount                   = uint32_t(stages.len()),
        .pStages                      = stages.data(),
        .pVertexInputState            = &state.vertex_info,
        .pInputAssemblyState          = &state.topology,
        .pViewportState               = &state.vs,
        .pRasterizationState          = &state.rs,
        .pMultisampleState            = &state.ms,
        .pDepthStencilState           = &state.ds,
        .pColorBlendState             = &state.blending,
        .layout                       = pipeline_layout,
        .renderPass                   = device->render_pass,
        .subpass                      = 0,
        .basePipelineHandle           = VK_NULL_HANDLE
    };
    ///
    if (vk_state)
        vk_state(&state);
    
    assert (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
}

/// create graphic pipeline
void PipelineMemory::update(size_t frame_id) {
    Frame         &frame = device->frames[frame_id];
    VkCommandBuffer &cmd = frame_commands[frame_id];
    auto     clear_color = [&](rgba c) {
        return VkClearColorValue {{ float(c->r) / 255.0f, float(c->g) / 255.0f,
                                    float(c->b) / 255.0f, float(c->a) / 255.0f }};
    };
    
    /// reallocate command
    vkFreeCommandBuffers(*device, device->command, 1, &cmd);
    auto alloc_info = VkCommandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        null, device->command, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    assert(vkAllocateCommandBuffers(*device, &alloc_info, &cmd) == VK_SUCCESS);
    
    /// begin command
    auto begin_info = VkCommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    assert(vkBeginCommandBuffer(cmd, &begin_info) == VK_SUCCESS);
    
    ///
    auto   clear_values = array<VkClearValue> {
        {        .color = clear_color(clr)}, // for image rgba  sfloat
        { .depthStencil = {1.0f, 0}}         // for image depth sfloat
    };
    
    /// nothing in canvas showed with depthStencil = 0.0.
    auto    render_info = VkRenderPassBeginInfo {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = null,
        .renderPass      = VkRenderPass(*device),
        .framebuffer     = VkFramebuffer(frame),
        .renderArea      = {{0,0}, device->extent},
        .clearValueCount = u32(clear_values.len()),
        .pClearValues    = clear_values.data()
    };

    /// gather some rendering ingredients
    vkCmdBeginRenderPass(cmd, &render_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    /// toss together a VBO array
    array<VkBuffer>  a_vbo   = { vbo };
    VkDeviceSize   offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, a_vbo.data(), offsets);
    auto ts = ibo->buffer->type_size;
    vkCmdBindIndexBuffer(cmd, ibo, 0, ts == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout, 0, 1, (const VkDescriptorSet*)desc_sets.data(), 0, nullptr);
    
    /// flip around an IBO and toss it in the oven at 410F
    u32 sz_draw = u32(ibo.len());
    vkCmdDrawIndexed(cmd, sz_draw, 1, 0, 0, 0); /// ibo size = number of indices (like a vector)
    vkCmdEndRenderPass(cmd);
    assert(vkEndCommandBuffer(cmd) == VK_SUCCESS);
}


/// the thing that 'executes' is the present.  so this is just 'update'
void Render::update() {
    Frame  &frame  = device->frames[cframe];
    assert (cframe == frame.index);
}

void Render::present() {
    Device &device = *this->device;
    vkWaitForFences(device, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index; /// look at the ref here
    VkResult result = vkAcquireNextImageKHR(
        device, device.swap_chain, UINT64_MAX,
        image_avail[cframe], VK_NULL_HANDLE, &image_index);
    ///Frame    &frame = device.frames[image_index];
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        assert(false); // lets see when this happens
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        assert(false); // lets see when this happens
    ///
    if (image_active[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &image_active[image_index], VK_TRUE, UINT64_MAX);
    ///
    vkResetFences(device, 1, &fence_active[cframe]);
    VkSemaphore             s_wait[] = {   image_avail[cframe] };
    VkSemaphore           s_signal[] = { render_finish[cframe] };
    VkPipelineStageFlags    f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    bool sync_diff = false;
    ///
    /// iterate through render sequence, updating out of sync pipelines
    while (sequence.size()) {
        auto &m = *(PipelineMemory*)sequence.front();
        sequence.pop();
        
        if (m.sync != sync) {
            /// Device::update() calls change the sync (so initial and updates)
            sync_diff = true;
            m.sync    = sync;
            if (!m.frame_commands)
                 m.frame_commands = array<VkCommandBuffer> (device.frames.len(), VK_NULL_HANDLE);
            for (size_t i = 0; i < device.frames.len(); i++) //
                m.update(i); /// make singular, frame index param
        }
        
        /// transfer uniform struct from lambda call. send us your uniforms!
        m.ubo->transfer(image_index);
        ///
        image_active[image_index]        = fence_active[cframe];
        VkSubmitInfo submit_info         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &m.frame_commands[image_index];
        submit_info.signalSemaphoreCount = 1;
        assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
        int test = 0;
        test++;
    }
    
    VkPresentInfoKHR     present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1,
        s_signal, 1, &device.swap_chain, &image_index };
    VkResult             presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
    
    if ((presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR)) {// && !sync_diff) {
        device.update(); /// cframe needs to be valid at its current value
    }// else
     //   assert(result == VK_SUCCESS);
    
    cframe = (cframe + 1) % MAX_FRAMES_IN_FLIGHT; /// ??
}

Render::Render(Device *device): device(device) {
    if (device) {
        const size_t ns  = device->swap_images.len();
        const size_t mx  = MAX_FRAMES_IN_FLIGHT;
        image_avail    = array<VkSemaphore>(mx);
        render_finish  = array<VkSemaphore>(mx);
        fence_active   = array<VkFence>    (mx);
        image_active   = array<VkFence>    (ns);
        ///
        VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
        for (size_t i = 0; i < mx; i++)
            assert (vkCreateSemaphore(*device, &si, null,   &image_avail[i]) == VK_SUCCESS &&
                    vkCreateSemaphore(*device, &si, null, &render_finish[i]) == VK_SUCCESS &&
                    vkCreateFence(    *device, &fi, null,  &fence_active[i]) == VK_SUCCESS);
        image_avail  .set_size(mx);
        render_finish.set_size(mx);
        fence_active .set_size(mx);
        image_active .set_size(ns);
    }
}

size_t IndexData::len() {
    return imem->buffer->sz / imem->buffer->type_size;
}

IndexData::IndexData(Device *device, Buffer buffer) : IndexData() {
    imem->device = device;
    imem->buffer = buffer;
}

VkBufferUsageFlags buffer_usage(Buffer::Usage usage) {
    switch (usage.value) {
        case Buffer::Usage::Undefined: return 0x00;
        case Buffer::Usage::Src:       return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        case Buffer::Usage::Dst:       return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case Buffer::Usage::Uniform:   return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case Buffer::Usage::Storage:   return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        case Buffer::Usage::Index:     return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case Buffer::Usage::Vertex:    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    return 0;
}

Buffer::Usage buffer_usage(VkBufferUsageFlags usage) {
    switch (usage) {
        case  VK_BUFFER_USAGE_TRANSFER_SRC_BIT:   return Buffer::Usage::Src;
        case  VK_BUFFER_USAGE_TRANSFER_DST_BIT:   return Buffer::Usage::Dst;
        case  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT: return Buffer::Usage::Uniform;
        case  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT: return Buffer::Usage::Storage;
        case  VK_BUFFER_USAGE_INDEX_BUFFER_BIT:   return Buffer::Usage::Index;
        case  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:  return Buffer::Usage::Vertex;
    }
    return Buffer::Usage::Undefined;
}

IndexData::operator bool () { return (*this)->device != null; }
bool IndexData::operator!() { return (*this)->device == null; }

BufferMemory *create_buffer(Device *d, size_t sz, void *bytes, type_t type, Buffer::Usage usage) {
    return new BufferMemory { d, sz, type->base_sz, bytes, buffer_usage(usage) };
}

BufferMemory::BufferMemory(Device *device, size_t sz, size_t type_size, void *bytes, 
    VkBufferUsageFlags usage, VkMemoryPropertyFlags pflags):
        device(device), sz(sz), type_size(type_size), usage(usage), pflags(pflags) {
    
    VkBufferCreateInfo bi {};
    bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size         = VkDeviceSize(sz * type_size); /// must have type-size as well; todo: make sure sz does not contain type-size elsewhere
    bi.usage        = usage;
    bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;
    ///
    console.test(vkCreateBuffer(*device, &bi, nullptr, &buffer) == VK_SUCCESS);
    
    /// fetch 'requirements'
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(*device, buffer, &req);
    
    /// allocate and bind
    VkMemoryAllocateInfo alloc { };
    alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize    = req.size;
    alloc.memoryTypeIndex   = memory_type(*device, req.memoryTypeBits, pflags);
    vkAllocateMemory(*device, &alloc, nullptr, &memory);
    vkBindBufferMemory(*device, buffer, memory, VkDeviceSize(0));
    
    ///
    info = VkDescriptorBufferInfo {
        .buffer = buffer,
        .offset = 0,
        .range  = sz * type_size
    };
}

void BufferMemory::copy_to(TextureMemory *tmem) {
    auto device = *this->device;
    auto cmd    = device.begin();
    auto reg    = VkBufferImageCopy {};
    reg.imageSubresource = { tmem->aflags, 0, 0, 1 };
    reg.imageExtent      = { uint32_t(tmem->sz[1]), uint32_t(tmem->sz[0]), 1 };
    vkCmdCopyBufferToImage(cmd, buffer, tmem->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
    device.submit(cmd);
}

void BufferMemory::copy_to(BufferMemory *dst, size_t size) {
    auto cb = device->begin();
    VkBufferCopy params { };
    params.size = VkDeviceSize(size);
    vkCmdCopyBuffer(cb, *this, *dst, 1, &params);
    device->submit(cb);
}

size_t Buffer::count()     { return bmem->sz; }
size_t Buffer::type_size() { return bmem->type_size; }

UniformData::UniformData(Device *device, size_t struct_sz, UniformFn fn)
    : mx(memory::wrap<UniformMemory>(
        new UniformMemory {
            .device    = device,
            .struct_sz = struct_sz,
            .fn        = fn
        }
    )), umem(data<UniformMemory>()) { }

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessageTypeFlagsEXT             type,
            const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
            void*                                       user_data) {
    std::cerr << "validation layer: " << cb_data->pMessage << std::endl;

    return VK_FALSE;
}

/// bring in gpu boot strapping from pascal project
array<VkPhysicalDevice> available_gpus() {
    vk_interface vk;
    VkInstance inst = vk.instance();
    u32 gpu_count;
    
    /// gpu count
    vkEnumeratePhysicalDevices(inst, &gpu_count, null);

    /// allocate arrays for gpus in static form
    static array<VkPhysicalDevice> hw(gpu_count, gpu_count, VK_NULL_HANDLE);
    static bool init;
    if (!init) {
        init = true;
        vkEnumeratePhysicalDevices(inst, &gpu_count, (VkPhysicalDevice*)hw.data());
    }
    return hw;
}

//#include <gpu/gl/GrGLInterface.h>

struct Internal {
    window  *win;
    VkInstance vk = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dmsg;
    array<VkLayerProperties> layers;
    bool     resize;
    array<VkFence> fence;
    array<VkFence> image_fence;
    Internal &bootstrap();
    void destroy();
    static Internal &handle();
};

Internal _;

Internal &Internal::handle()    { return _.vk ? _ : _.bootstrap(); }

void Internal::destroy() {
    // Destroy the debug messenger and vk instance when finished
    DestroyDebugUtilsMessengerEXT(vk, dmsg, null);
    vkDestroyInstance(vk, null);
}

Internal &Internal::bootstrap() {
    static bool init;
    if (!init) {
        init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    //uint32_t count;
    //vkEnumerateInstanceLayerProperties(&count, nullptr);
    //layers.set_size(count);
    //vkEnumerateInstanceLayerProperties(&count, layers.data());

    const symbol validation_layer   = "VK_LAYER_KHRONOS_validation";
    u32          glfwExtensionCount = 0;
    symbol*      glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    array<symbol> extensions { &glfwExtensions[0], glfwExtensionCount, glfwExtensionCount + 1 };

    extensions += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    if constexpr (is_apple())
        extensions += VK_EXT_METAL_SURFACE_EXTENSION_NAME;

    for (symbol sym: extensions) {
        console.log("symbol: {0}", { sym });
    }

    // set the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debug {
        .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback    = vk_debug
    };

    // set the application info
    VkApplicationInfo app_info   = {
        .sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName    = "ion",
        .applicationVersion  = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName         = "ion",
        .engineVersion       = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion          = VK_API_VERSION_1_2
    };

    VkInstanceCreateInfo create_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debug,
        .pApplicationInfo        = &app_info,
        .flags                   = VkInstanceCreateFlags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR),
        .enabledLayerCount       = is_debug() ? 1 : 0,
        .ppEnabledLayerNames     = &validation_layer,
        .enabledExtensionCount   = u32(extensions.len()),
        .ppEnabledExtensionNames = extensions.data()
    };

    // create the vk instance (MoltenVK does not support validation layer)
    // M2 mac mini reports incompatible driver
    VkResult vk_result = vkCreateInstance(&create_info, null, &vk);

    /// if layer not present, attempt to load without
    if (is_debug() && vk_result == VK_ERROR_LAYER_NOT_PRESENT) {
        console.log("warning: validation layer unavailable (VK_ERROR_LAYER_NOT_PRESENT)");
        create_info.enabledLayerCount = 0;
        vk_result = vkCreateInstance(&create_info, null, &vk);
    }
    /// test for success
    console.test(vk_result == VK_SUCCESS, "vk instance failure");

    /// create the debug messenger
    VkResult dbg_result = CreateDebugUtilsMessengerEXT(vk, &debug, null, &dmsg);
    console.test(dbg_result == VK_SUCCESS, "vk debug messenger failure");
    return *this;
}

/// cleaning up the vk_instance / internal stuff now
Texture &window::texture() { return *w->tx_skia; }
Texture &window::texture(::size sz) {
    if (w->tx_skia) {
        delete w->tx_skia;
    }
    ///  implicit wrap case, this takes over the life-cycle.  when it runs out of refs and it has no attachments, it will free memory.
    w->tx_skia = new Texture {
        new TextureMemory {
            w->dev, sz, rgba { 1.0, 0.0, 1.0, 1.0 },
            VK_IMAGE_USAGE_SAMPLED_BIT          | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT     | VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_B8G8R8A8_UNORM, -1
        }
    };
    w->tx_skia->push_stage(Texture::Stage::Color);
    return *w->tx_skia;
}

vk_interface::vk_interface() : i(&Internal::handle()) { }
///
//::window    &vk_interface::window  () { return *i->win;        }
//::Device    &vk_interface::device  () { return *i->win->w->dev; }
VkInstance    &vk_interface::instance() { return  i->vk;         }
uint32_t       vk_interface::version () { return VK_MAKE_VERSION(1, 1, 0); }
Device        *window::device()         { return w->dev; }


void glfw_key(GLFWwindow *h, int unicode, int scan_code, int action, int mods) {
    window win = window::handle(h);

    /// update modifiers on window
    win.w->ev->modifiers[keyboard::shift] = (mods & GLFW_MOD_SHIFT) != 0;
    win.w->ev->modifiers[keyboard::alt]   = (mods & GLFW_MOD_ALT  ) != 0;
    win.w->ev->modifiers[keyboard::meta]  = (mods & GLFW_MOD_SUPER) != 0;

    win.w->key_event(
        user::key::data {
            .unicode   = unicode,
            .scan_code = scan_code,
            .modifiers = win.w->ev->modifiers,
            .repeat    = action == GLFW_REPEAT,
            .up        = action == GLFW_RELEASE
        }
    );
}

void glfw_char(GLFWwindow *h, u32 unicode) { window::handle(h).w->char_event(unicode); }

void glfw_button(GLFWwindow *h, int button, int action, int mods) {
    window win = window::handle(h);
    switch (button) {
        case 0: win.w->ev->buttons[ mouse::left  ] = action == GLFW_PRESS; break;
        case 1: win.w->ev->buttons[ mouse::right ] = action == GLFW_PRESS; break;
        case 2: win.w->ev->buttons[ mouse::middle] = action == GLFW_PRESS; break;
    }
    win.w->button_event(win.w->ev->buttons);
}

void glfw_cursor (GLFWwindow *h, double x, double y) {
    window win = window::handle(h);
    win.w->ev->cursor = vec2 { x, y };
    win.w->ev->buttons[mouse::inactive] = !glfwGetWindowAttrib(h, GLFW_HOVERED);
    win.w->cursor_event(win.w->ev->cursor);
}

void glfw_resize (GLFWwindow *handle, i32 w, i32 h) {
    window win = window::handle(handle);
    win.w->fn_resize();
    win.w->sz = size { h, w };
    win.w->resize_event(win.w->sz);
}

bool GPU::operator()(Capability caps) {
    bool g = gmem->gfx.type()     == typeof(u32);
    bool p = gmem->present.type() == typeof(u32);
    if (caps == Complete) return g && p;
    if (caps == Graphics) return g;
    if (caps == Present)  return p;
    return true;
}

uint32_t GPU::index(Capability caps) {
    assert(gmem->gfx.type() == typeof(u32) || gmem->present.type() == typeof(u32));
    if (caps == Graphics && (*this)(Graphics)) return u32(gmem->gfx);
    if (caps == Present  && (*this)(Present))  return u32(gmem->present);
    assert(false);
    return 0;
}

GPU::operator bool() {
    return (*this)(Complete) &&
        gmem->support.formats &&
        gmem->support.present_modes &&
        gmem->support.ansio;
}

Assets Pipes::cache_assets(Pipes::data &d, str model, str skin, states<Asset> &atypes) {
    auto   &cache = d.device->tx_cache; /// ::map<Path, Device::Pair>
    Assets assets = Assets(Asset::count);
    ///
    auto load_tx = [&](Path p) -> Texture * {
        /// load texture if not already loaded
        if (!cache.count(p)) {
            cache[p].img      = new image(p);
            cache[p].texture  = new Texture {
                new TextureMemory { d.device, *cache[p].img,
                    VK_IMAGE_USAGE_SAMPLED_BIT      | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_R8G8B8A8_UNORM, -1
                }
            };
            cache[p].texture->push_stage(Texture::Stage::Shader);
        };
        return cache[p].texture;
    };
    
    #if 0
    static auto names = ::map<str> {
        { Asset::Color,    "color"    },
        { Asset::Specular, "specular" },
        { Asset::Displace, "displace" },
        { Asset::Normal,   "normal"   }
    };
    #endif

    doubly<memory*> &enums = Asset::symbols();
    ///
    for (memory *sym:enums) {
        str         name = sym->symbol(); /// no allocation and it just holds onto this symbol memory, not mutable
        Asset type { Asset::etype(sym->id) };
        if (!atypes[type]) continue;
        /// prefer png over jpg, if either one exists
        Path png = form_path(model, skin, ".png");
        Path jpg = form_path(model, skin, ".jpg");
        if (!png.exists() && !jpg.exists()) continue;
        assets[type] = load_tx(png.exists() ? png : jpg);
    }
    return assets;
}

#if 0
/// todo: vulkan render to texture
void sk_canvas_gaussian(sk_canvas_data* sk_canvas, v2r* sz, r4r* crop) {
    SkPaint  &sk     = *sk_canvas->state->backend.sk_paint;
    SkCanvas &canvas = *sk_canvas->sk_canvas;
    ///
    SkImageFilters::CropRect crect = { };
    if (crop && crop->w > 0 && crop->h > 0) {
        SkRect rect = { SkScalar(crop->x),          SkScalar(crop->y),
                        SkScalar(crop->x + crop->w), SkScalar(crop->y + crop->h) };
        crect       = SkImageFilters::CropRect(rect);
    }
    sk_sp<SkImageFilter> filter = SkImageFilters::Blur(sks(sz->x), sks(sz->y), nullptr, crect);
    sk.setImageFilter(std::move(filter));
}
#endif

struct gfx_memory {
    VkvgSurface    vg_surface;
    VkvgDevice     vg_device;
    VkvgContext    ctx;
    VkhDevice      vkh_device;
    VkhImage       vkh_image;
    str            font_default;
    window        *win;
    Texture        tx;
    cbase::draw_state *ds;

    ~gfx_memory() {
        if (ctx)        vkvg_destroy        (ctx);
        if (vg_device)  vkvg_device_destroy (vg_device);
        if (vg_surface) vkvg_surface_destroy(vg_surface);
    }
};

ptr_impl(gfx, cbase, gfx_memory, g);

gfx::gfx(::window &win) : gfx(memory::wrap<gfx_memory>(new gfx_memory { })) {
    vk_interface vk; /// verify vulkan instanced prior
    g->win = &win;    /// should just set window without pointer
    m.size = win.w->sz;
    assert(m.size[1] > 0 && m.size[0] > 0);

    /// erases old main texture, returns the newly sized.
    /// this is 'texture-main' of sorts accessible from canvas context2D
    g->tx = win.texture(m.size); 

    VkInstance vk_inst = vk.instance();
    Device *device = win.device();

    u32   family_index = device->gpu.index(GPU::Graphics);//win.graphics_index();
    u32    queue_index = 0; // not very clear this.

    
    TextureMemory *tmem = g->tx;
  // not sure if the import is getting the VK_SAMPLE_COUNT_8_BIT and VkSampler, likely not.
    g->vg_device       = vkvg_device_create_from_vk_multisample(
        vk_inst, *device, *device,
        family_index, queue_index, VK_SAMPLE_COUNT_8_BIT, false);
    g->vkh_device      = vkh_device_import(vk.instance(), *device, *device); // use win instead of vk.
    g->vkh_image       = vkh_image_import(g->vkh_device, tmem->vk_image, tmem->format, tmem->sz[1], tmem->sz[0]);

    vkh_image_create_view(g->vkh_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT); // VK_IMAGE_ASPECT_COLOR_BIT questionable bit
    g->vg_surface       = vkvg_surface_create_for_VkhImage(g->vg_device, (void*)g->vkh_image);
    g->ctx              = vkvg_create(g->vg_surface);
    push(); /// gfxs just needs a push off the ledge. [/penguin-drops]
    defaults();
}

window::window(::size sz, mode::etype wmode, memory *control) : window() {
    /*
    static bool is_init = false;
    if(!is_init) {
        is_init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    */
    VkInstance vk = Internal::handle().vk;
    w->sz          = sz;
    w->headless    = wmode == mode::headless;
    w->control     = control->grab();
    w->glfw        = glfwCreateWindow(sz[1], sz[0], (symbol)"ux-vk", null, null);
    w->vk_surface  = 0;
    
    VkResult code = glfwCreateWindowSurface(vk, w->glfw, null, &w->vk_surface);

    console.test(code == VK_SUCCESS, "initial surface creation failure.");

    /// set glfw callbacks
    glfwSetWindowUserPointer(w->glfw, (void*)mem->grab()); /// this is another reference on the window, which must be unreferenced when the window closes
    glfwSetErrorCallback    (glfw_error);

    if (!w->headless) {
        glfwSetFramebufferSizeCallback(w->glfw, glfw_resize);
        glfwSetKeyCallback            (w->glfw, glfw_key);
        glfwSetCursorPosCallback      (w->glfw, glfw_cursor);
        glfwSetCharCallback           (w->glfw, glfw_char);
        glfwSetMouseButtonCallback    (w->glfw, glfw_button);
    }

    array<VkPhysicalDevice> hw = available_gpus();

    /// a fast algorithm for choosing top gpu
    const size_t top_pick = 0; 
    w->gpu = create_gpu(hw[top_pick], w->vk_surface);

    /// create device with window, selected gpu and surface
    w->dev = create_device(*w->gpu, true); 
    w->dev->initialize(w);
}

void vkvg_path(VkvgContext ctx, memory *mem) {
    type_t ty = mem->type;
    
    if (ty == typeof(r4<real>)) {
        r4<real> &m = mem->ref<r4<real>>();
        vkvg_rectangle(ctx, m.x, m.y, m.w, m.h);
    }
    else if (ty == typeof(graphics::rounded::data)) {
        graphics::rounded::data &m = mem->ref<graphics::rounded::data>();
        vkvg_move_to (ctx, m.tl_x.x, m.tl_x.y);
        vkvg_line_to (ctx, m.tr_x.x, m.tr_x.y);
        vkvg_curve_to(ctx, m.c0.x,   m.c0.y, m.c1.x, m.c1.y, m.tr_y.x, m.tr_y.y);
        vkvg_line_to (ctx, m.br_y.x, m.br_y.y);
        vkvg_curve_to(ctx, m.c0b.x,  m.c0b.y, m.c1b.x, m.c1b.y, m.br_x.x, m.br_x.y);
        vkvg_line_to (ctx, m.bl_x.x, m.bl_x.y);
        vkvg_curve_to(ctx, m.c0c.x,  m.c0c.y, m.c1c.x, m.c1c.y, m.bl_y.x, m.bl_y.y);
        vkvg_line_to (ctx, m.tl_y.x, m.tl_y.y);
        vkvg_curve_to(ctx, m.c0d.x,  m.c0d.y, m.c1d.x, m.c1d.y, m.tl_x.x, m.tl_x.y);
    }
    else if (ty == typeof(graphics::shape::data)) {
        graphics::shape::data &m = mem->ref<graphics::shape::data>();
        for (mx &o:m.ops) {
            type_t t = o.type();
            if (t == typeof(graphics::movement)) {
                graphics::movement m(o);
                vkvg_move_to(ctx, m.data.x, m.data.y); /// todo: convert vkvg to double.  simply not using float!
            } else if (t == typeof(vec2)) {
                vec2 l(o);
                vkvg_line_to(ctx, l.data.x, l.data.y);
            }
        }
        vkvg_close_path(ctx);
    }
}

::window &gfx::window() { return *g->win; }
Device   &gfx::device() { return *g->win->w->dev; }

/// Quake2: { computer. updated. }
void gfx::draw_state_change(draw_state *ds, cbase::state_change type) {
    g->ds = ds;
}

text_metrics gfx::measure(str text) {
    vkvg_text_extents_t ext;
    vkvg_font_extents_t tm;
    ///
    vkvg_text_extents(g->ctx, (symbol)text.cs(), &ext);
    vkvg_font_extents(g->ctx, &tm);
    return text_metrics::data {
        .w           =  real(ext.x_advance),
        .h           =  real(ext.y_advance),
        .ascent      =  real(tm.ascent),
        .descent     =  real(tm.descent),
        .line_height =  real(g->ds->line_height)
    };
}

str gfx::format_ellipsis(str text, real w, text_metrics &tm_result) {
    symbol       t  = (symbol)text.cs();
    text_metrics tm = measure(text);
    str result;
    ///
    if (tm->w <= w)
        result = text;
    else {
        symbol e       = "...";
        size_t e_len   = strlen(e);
        r32    e_width = measure(e)->w;
        size_t cur_w   = 0;
        ///
        if (w <= e_width) {
            size_t len = text.len();
            while (len > 0 && tm->w + e_width > w)
                tm = measure(text.mid(0, int(--len)));
            if (len == 0)
                result = str(cstr(e), e_len);
            else {
                cstr trunc = (cstr)malloc(len + e_len + 1);
                strncpy(trunc, text.cs(), len);
                strncpy(trunc + len, e, e_len + 1);
                result = str(trunc, len + e_len);
                free(trunc);
            }
        }
    }
    tm_result = tm;
    return result;
}

void gfx::draw_ellipsis(str text, real w) {
    text_metrics tm;
    str draw = format_ellipsis(text, w, tm);
    if (draw)
        vkvg_show_text(g->ctx, (symbol)draw.cs());
}

void gfx::image(::image img, graphics::shape sh, vec2 align, vec2 offset, vec2 source) {
    attachment *att = img.find_attachment("vg-surf");
    if (!att) {
        VkvgSurface surf = vkvg_surface_create_from_bitmap(
            g->vg_device, (uint8_t*)img.pixels(), img.width(), img.height());
        att = img.attach("vg-surf", surf, [surf]() {
            vkvg_surface_destroy(surf);
        });
        assert(att);
    }
    VkvgSurface surf = (VkvgSurface)att->data;
    draw_state   &ds = *g->ds;
    ::size        sz = img.shape();
    r4r           &r = sh.rect();
    assert(surf);
    vkvg_set_source_rgba(g->ctx,
        ds.color->r, ds.color->g, ds.color->b, ds.color->a * ds.opacity);
    
    /// now its just of matter of scaling the little guy to fit in the box.
    v2<real> vsc = { math::min(1.0, r.w / real(sz[1])), math::min(1.0, r.h / real(sz[0])) };
    real      sc = (vsc.y > vsc.x) ? vsc.x : vsc.y;
    vec2     pos = { mix(r.x, r.x + r.w - sz[1] * sc, align.data.x),
                        mix(r.y, r.y + r.h - sz[0] * sc, align.data.y) };
    
    push();
    /// translate & scale
    translate(pos + offset);
    scale(sc);
    /// push path
    vkvg_rectangle(g->ctx, r.x, r.y, r.w, r.h);
    /// color & fill
    vkvg_set_source_surface(g->ctx, surf, source.data.x, source.data.y);
    vkvg_fill(g->ctx);
    pop();
}

void gfx::push() {
    cbase::push();
    vkvg_save(g->ctx);
}

void gfx::pop() {
    cbase::pop();
    vkvg_restore(g->ctx);
}

/// would be reasonable to have a rich() method
/// the lines are most definitely just text() calls, it should be up to the user to perform multiline.
///
/// ellipsis needs to be based on align
/// very important to make the canvas have good primitive ops, to allow the user 
/// to leverage high drawing abstracts direct from canvas, make usable everywhere!
///
/// rect for the control area, region for the sizing within
/// important that this info be known at time of output where clipping and ellipsis is concerned
/// 
void gfx::text(str text, graphics::rect rect, alignment align, vec2 offset, bool ellip) {
    draw_state &ds = *g->ds;
    rgba::data & c = ds.color;
    ///
    vkvg_save(g->ctx);
    vkvg_set_source_rgba(g->ctx, c.r, c.g, c.b, c.a * ds.opacity);
    ///
    v2r            pos  = { 0, 0 };
    text_metrics   tm;
    str            txt;
    ///
    if (ellip) {
        txt = format_ellipsis(text, rect->w, tm);
        /// with -> operator you dont need to know the data ref.
        /// however, it looks like a pointer.  the bigger the front, the bigger the back.
        /// in this case.  they equate to same operations
    } else {
        txt = text;
        tm  = measure(txt);
    }

    /// its probably useful to have a line height in the canvas
    real line_height = (-tm->descent + tm->ascent) / 1.66;
    v2r           tl = { rect->x,  rect->y + line_height / 2 };
    v2r           br = { rect->x + rect->w - tm->w,
                            rect->y + rect->h - tm->h - line_height / 2 };
    v2r           va = vec2(align);
    pos              = mix(tl, br, va);
    
    vkvg_show_text(g->ctx, (symbol)text.cs());
    vkvg_restore  (g->ctx);
}

void gfx::clip(graphics::shape cl) {
    draw_state  &ds = cur();
    ds.clip  = cl;
    vkvg_path(g->ctx, cl.mem);
    vkvg_clip(g->ctx);
}

Texture gfx::texture() { return g->tx; } /// associated with surface

void gfx::flush() {
    vkvg_flush(g->ctx);
}

void gfx::clear(rgba c) {
    vkvg_save           (g->ctx);
    vkvg_set_source_rgba(g->ctx, c->r, c->g, c->b, c->a);
    vkvg_paint          (g->ctx);
    vkvg_restore        (g->ctx);
}

void gfx::font(::font f) {
    vkvg_select_font_face(g->ctx, f->alias.cs());
    vkvg_set_font_size   (g->ctx, f->sz);
}

void gfx::cap  (graphics::cap   c) { vkvg_set_line_cap (g->ctx, vkvg_line_cap_t (int(c))); }
void gfx::join (graphics::join  j) { vkvg_set_line_join(g->ctx, vkvg_line_join_t(int(j))); }
void gfx::translate(vec2       tr) { vkvg_translate    (g->ctx, tr->x, tr->y);  }
void gfx::scale    (vec2       sc) { vkvg_scale        (g->ctx, sc->x, sc->y);  }
void gfx::scale    (real       sc) { vkvg_scale        (g->ctx, sc, sc);        }
void gfx::rotate   (real     degs) { vkvg_rotate       (g->ctx, radians(degs)); }
void gfx::fill(graphics::shape  p) {
    vkvg_path(g->ctx, p.mem);
    vkvg_fill(g->ctx);
}

void gfx::gaussian (vec2 sz, graphics::shape c) { }

void gfx::outline  (graphics::shape p) {
    vkvg_path(g->ctx, p.mem);
    vkvg_stroke(g->ctx);
}

void*    gfx::data      ()                   { return null; }
str      gfx::get_char  (int x, int y)       { return str(); }
str      gfx::ansi_color(rgba &c, bool text) { return str(); }

::image  gfx::resample  (::size sz, real deg, graphics::shape view, vec2 axis) {
    c4<u8> *pixels = null;
    int scanline = 0;
    return ::image(sz, (rgba::data*)pixels, scanline);
}

int app::run() {
    /// this was debugging a memory issue
    for (size_t i = 0; i < 100; i++) {
        m.win    = window {size { 512, 512 }, mode::regular, mx::mem };
        m.canvas = gfx(m.win);
    }
    m.win    = window {size { 512, 512 }, mode::regular, mx::mem };
    m.canvas = gfx(m.win);
    Device &dev = m.canvas.device();
    window &win = m.canvas.window();

    ///
    auto   vertices = Vertex::square (); /// doesnt always get past this.
    auto    indices = array<uint16_t> { 0, 1, 2, 2, 3, 0 };
    auto   textures = Assets {{ Asset::Color, &win.texture() }}; /// quirk in that passing Asset::Color enumerable does not fetch memory with id, but rather memory of int(id)
    auto      vattr = VAttribs { VA::Position, VA::Normal, VA::UV, VA::Color };
    auto        vbo = VertexBuffer<Vertex>  { &dev, vertices, vattr };
    auto        ibo = IndexBuffer<uint16_t> { &dev, indices  };
    auto        uni = UniformBuffer<MVP>    { &dev, [&](void *mvp) {
        *(MVP*)mvp  = MVP {
             .model = m44f::ident(), //m44f::identity(),
             .view  = m44f::ident(), //m44f::identity(),
             .proj  = m44f::ortho({ -0.5f, 0.5f }, { -0.5f, 0.5f }, { 0.5f, -0.5f })
        };
    }}; /// assets dont seem to quite store Asset::Color and its texture ref
    auto         pl = Pipeline<Vertex> {
        &dev, uni, vbo, ibo, textures,
        { 0.0, 0.0, 0.0, 0.0 }, std::string("main") /// transparent canvas overlay; are we clear? crystal.
    };

    listener &on_key = win.w->events.on_key.listen(
        [&](event e) {
            win.w->pristine = false;
            win.w->ev->key  = e->key; /// dont need to store button_id, thats just for handlers. window is stateless
        });
    
    listener &on_button = win.w->events.on_button.listen(
        [&](event e) {
            win.w->pristine    = false;
            win.w->ev->buttons = e->buttons;
        });

    listener &on_cursor = win->events.on_cursor.listen(
        [&](event e) {
            win.w->pristine   = false;
            win.w->ev->cursor = e->cursor;
        });

    ///
    win.show();
    
    /// prototypes add a Window&
    /// w->fn_cursor  = [&](double x, double y)         { composer->cursor(w, x, y);    };
    /// w->fn_mbutton = [&](int b, int a, int m)        { composer->button(w, b, a, m); };
    /// w->fn_resize  = [&]()                           { composer->resize(w);          };
    /// w->fn_key     = [&](int k, int s, int a, int m) { composer->key(w, k, s, a, m); };
    /// w->fn_char    = [&](uint32_t c)                 { composer->character(w, c);    };
    
    /// uniforms are meant to be managed by the app, passed into pipelines.
    win.loop([&]() {
        Element e = m.app_fn(*this);
        //canvas.clear(rgba {0.0, 0.0, 0.0, 0.0});
        //canvas.flush();

        // this should only paint the 3D pipeline, no canvas
        //i.tx_skia.push_stage(Texture::Stage::Shader);
        //device.render.push(pl); /// Any 3D object would have passed its pipeline prior to this call (in render)
        dev.render.present();
        //i.tx_skia.pop_stage();
        
        //if (composer->d.root) w->set_title(composer->d.root->m.text.label);

        // process animations
        //composer->process();
        
        //glfwWaitEventsTimeout(1.0);
    });

    /// automatic when it falls out of scope
    on_cursor.cancel();

    glfwTerminate();
    return 0;
}


/*
    member data: (member would be map<style_value> )

    StyleValue
        NMember<T, MT> *host         = null; /// needs to be 
        transition     *trans        = null;
        bool            manual_trans = false;
        int64_t         timer_start  = 0;
        sh<T>           t_start;       // transition start value (could be in middle of prior transition, for example, whatever its start is)
        sh<T>           t_value;       // current transition value
        sh<T>           s_value;       // style value
        sh<T>           d_value;       // default value

    size_t        cache = 0;
    sh<T>         d_value;
    StyleValue    style;
*/

// we just need to make compositor with transitions a bit cleaner than previous attempt.  i want to use all mx-pattern




bool ws(cstr &cursor) {
    auto is_cmt = [](symbol c) -> bool { return c[0] == '/' && c[1] == '*'; };
    while (isspace(*cursor) || is_cmt(cursor)) {
        while (isspace(*cursor))
            cursor++;
        if (is_cmt(cursor)) {
            cstr f = strstr(cursor, "*/");
            cursor = f ? &f[2] : &cursor[strlen(cursor) - 1];
        }
    }
    return *cursor != 0;
}

bool scan_to(cstr &cursor, array<char> chars) {
    bool sl  = false;
    bool qt  = false;
    bool qt2 = false;
    for (; *cursor; cursor++) {
        if (!sl) {
            if (*cursor == '"')
                qt = !qt;
            else if (*cursor == '\'')
                qt2 = !qt2;
        }
        sl = *cursor == '\\';
        if (!qt && !qt2)
            for (char &c: chars)
                if (*cursor == c)
                    return true;
    }
    return false;
}

doubly<style::qualifier> parse_qualifiers(cstr *p) {
    str   qstr;
    cstr start = *p;
    cstr end   = null;
    cstr scan  =  start;
    
    /// read ahead to {
    do {
        if (!*scan || *scan == '{') {
            end  = scan;
            qstr = str(start, std::distance(start, scan));
            break;
        }
    } while (*++scan);
    
    ///
    if (!qstr) {
        end = scan;
        *p  = end;
        return null;
    }
    
    ///
    auto  quals = qstr.split(",");
    doubly<style::qualifier> result;

    ///
    for (str &qs:quals) {
        str  q = qs.trim();
        if (!q) continue;
        style::qualifier &v = result.push();
        int idot = q.index_of(".");
        int icol = q.index_of(":");
        str tail;
        ///
        if (idot >= 0) {
            array<str> sp = q.split(".");
            v.data.type   = sp[0];
            v.data.id     = sp[1];
            if (icol >= 0)
                tail  = q.mid(icol + 1).trim();
        } else {
            if (icol  >= 0) {
                v.data.type = q.mid(0, icol);
                tail   = q.mid(icol + 1).trim();
            } else
                v.data.type = q;
        }
        array<str> ops {"!=",">=","<=",">","<","="};
        if (tail) {
            // check for ops
            bool is_op = false;
            for (str &op:ops) {
                if (tail.index_of(op.cs()) >= 0) {
                    is_op   = true;
                    auto sp = tail.split(op);
                    v.data.state = sp[0].trim();
                    v.data.oper  = op;
                    v.data.value = tail.mid(sp[0].len() + op.len()).trim();
                    break;
                }
            }
            if (!is_op)
                v.data.state = tail;
        }
    }
    *p = end;
    return result;
}

void style::cache_members() {
    lambda<void(block &)> cache_b;
    ///
    cache_b = [&](block &bl) -> void {
        for (entry &e: bl->entries) {
            bool  found = false;
            ///
            array<block> &cache = members(e->member);
            for (block &cb:cache)
                 found |= cb == bl;
            ///
            if (!found)
                 cache += bl;
        }
        for (block &s:bl->blocks)
            cache_b(s);
    };
    if (m.root)
        for (block &b:m.root)
            cache_b(b);
}

style::style(str code) : style(mx::alloc<style>()) {
    ///
    if (code) {
        for (cstr sc = code.cs(); ws(sc); sc++) {
            lambda<void(block)> parse_block;
            ///
            parse_block = [&](block bl) {
                ws(sc);
                console.test(*sc == '.' || isalpha(*sc), "expected Type, or .name");
                bl->quals = parse_qualifiers(&sc);
                ws(++sc);
                ///
                while (*sc && *sc != '}') {
                    /// read up to ;, {, or }
                    ws(sc);
                    cstr start = sc;
                    console.test(scan_to(sc, {';', '{', '}'}), "expected member expression or qualifier");
                    if (*sc == '{') {
                        ///
                        block &bl_n = bl->blocks.push();
                        bl_n->parent = bl;

                        /// parse sub-block
                        sc = start;
                        parse_block(bl_n);
                        assert(*sc == '}');
                        ws(++sc);
                        ///
                    } else if (*sc == ';') {
                        /// read member
                        cstr cur = start;
                        console.test(scan_to(cur, {':'}) && (cur < sc), "expected [member:]value;");
                        str  member = str(start, std::distance(start, cur));
                        ws(++cur);

                        /// read value
                        cstr vstart = cur;
                        console.test(scan_to(cur, {';'}), "expected member:[value;]");
                        
                        /// this should use the regex standard api, will convert when its feasible.
                        str  cb_value = str(vstart, std::distance(vstart, cur)).trim();
                        str       end = cb_value.mid(-1, 1);
                        bool       qs = cb_value.mid( 0, 1) == "\"";
                        bool       qe = cb_value.mid(-1, 1) == "\"";

                        if (qs && qe) {
                            cstr   cs = cb_value.cs();
                            cb_value  = str::parse_quoted(&cs, cb_value.len());
                        }

                        int         i = cb_value.index_of(",");
                        str     param = i >= 0 ? cb_value.mid(i + 1).trim() : "";
                        str     value = i >= 0 ? cb_value.mid(0, i).trim()  : cb_value;
                        style::transition trans = param  ? style::transition(param) : null;
                        
                        /// check
                        console.test(member, "member cannot be blank");
                        console.test(value,  "value cannot be blank");
                        bl->entries += entry::data { member, value, trans };
                        ws(++sc);
                    }
                }
                console.test(!*sc || *sc == '}', "expected closed-brace");
            };
            ///
            block &n_block = m.root.push_default();
            parse_block(n_block);
        }
        /// store blocks by member, the interface into all style: style::members[name]
        cache_members();
    }
}

style style::load(path p) {
    return cache.count(p) ? cache[p] : (cache[p] = style(p));
}

style style::for_class(symbol class_name) {
    path p = str::format("style/{0}.css", { class_name });
    return load(p);
}