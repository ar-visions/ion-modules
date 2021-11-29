#pragma once
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <dx/dx.hpp>
#include <media/canvas.hpp>
#include <media/obj.hpp>

struct Device;
struct Texture;
VkDevice                handle(Device *device);
VkDevice         device_handle(Device *device);
VkPhysicalDevice    gpu_handle(Device *device);
uint32_t           memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props);

#include <vk/window.hpp>
#include <vk/gpu.hpp>
#include <vk/device.hpp>
#include <vk/buffer.hpp> /// many buffers here. [tumbleweed near wooden sign]

struct Uniform {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

///
struct Texture {
    protected:
        void create(rgba *pixels);
        void create_sampler();
        void create_resources();
        ///
    public:
        Device                 *device     = null;
        VkImage                 image      = VK_NULL_HANDLE;
        VkImageView             view       = VK_NULL_HANDLE;
        VkDeviceMemory          memory     = VK_NULL_HANDLE;
        VkSampler               sampler    = VK_NULL_HANDLE;
        VkFormat                format;
        vec2i                   sz         = { 0, 0 };
        int                     mip_levels = 0;
        VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkImageUsageFlags       usage;
        VkImageAspectFlags      aflags;
        VkImageTiling           tiling;
        VkImageLayout           layout     = VK_IMAGE_LAYOUT_UNDEFINED; // needless monkeywork
        VkSampleCountFlagBits   msaa       = VK_SAMPLE_COUNT_1_BIT;
        ///
        void destroy();
        void set_layout(VkImageLayout new_layout);
        static VkImageView create_view(VkDevice, vec2i &, VkImage, VkFormat, VkImageAspectFlags, uint32_t mipLevels);
    
        ///
        /// we need to create the color resources with texture as well, simplify flags if possible
        /// int = mip levels
        static int get_mips(int mip_levels, vec2i sz);
        Texture(nullptr_t n = null) { }
        Texture(Device *, vec2i, rgba,
                VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, VkSampleCountFlagBits,
                VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Texture(Device *device, Image &im,
                VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, VkSampleCountFlagBits,
                VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Texture(Device *device, vec2i sz, VkImage image, VkImageView view,
                VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, VkSampleCountFlagBits,
                VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        ///
        operator VkDescriptorImageInfo() {
            return { sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        }
        ///
        operator  bool();
        bool operator!();
        operator VkImage &();
        operator VkImageView &();
        operator VkDeviceMemory &();
        operator VkAttachmentReference();
        operator VkAttachmentDescription();
};

/// What do you call this controller?
struct Frame {
    int                    index;
    Device                *device;
    Texture                tx_color, tx_depth, tx_swap; // Textures can be references to others created elsewhere, with views of their own instancing
    VkFramebuffer          framebuffer;
    /// shader used by Render
    UniformBuffer<Uniform> ubo; //
    Uniform                uniform;
    /// set uniforms by lambda service per pipeline instance
    map<str, VkCommandBuffer> render_commands;
    void destroy();
    void  create(vec<VkImageView> attachments);
    operator VkFramebuffer &() {
        return framebuffer;
    }
};

/// worth having; rid ourselves of more terms =)
struct Descriptor {
    Device *device;
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;
    operator VkDescriptorSetLayout &() { return layout; }
    operator VkDescriptorPool      &() { return pool;   }
    void destroy();
    Descriptor(Device *device = null) : device(device) { }
};

struct PipelineData;
struct Plumbing {
protected:
    Device *device;
    std::map<std::string, PipelineData *> pipelines;
public:
    PipelineData &operator[](std::string n);
    void update(Frame &frame);
    Plumbing(Device *device = null): device(device) { }
};

struct Render {
    Device          *device;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes perhaps
    vec<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    vec<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    vec<VkFence>     fence_active;
    vec<VkFence>     image_active;
    int              cframe = 0;
    bool             resized;
    ///
    Render(Device * = null);
    void present();
};

struct Window;
struct Device {
    void                  create_swapchain();
    void                  create_command_buffers();
    void                  create_render_pass();
    
    Render                render;
    VkSampleCountFlagBits msaa_samples;
    VkRenderPass          render_pass;
    Descriptor            desc;
    GPU                   gpu;
    VkCommandPool         pool;
    VkDevice              device;
    VkQueue               queues[GPU::Complete];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    vec<Frame>            frames;
    vec<VkImage>          swap_images;
    Plumbing              plumbing;
    VkSampleCountFlagBits sampling;
    Texture               tx_color;
    Texture               tx_depth;
    
    PipelineData &operator[](std::string n);
    ///
    void            initialize(Window *);
    /// todo: initialize for compute_only
    void            update();
    void            start();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    uint32_t        memory_type(uint32_t types, VkMemoryPropertyFlags props);
    
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    operator        VkRenderPass();
    Device(GPU &gpu, bool aa = false);
    VkQueue &operator()(GPU::Capability cap);
    ///
    Device(nullptr_t n = null) { }
    operator VkViewport &() { return viewport; }
    operator VkDescriptorSetLayout &() {
        return desc;
    }
    operator VkDescriptorPool &() {
        return desc;
    }
    operator VkPipelineLayoutCreateInfo() {
        return { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, null, 0, 1, &(VkDescriptorSetLayout &)desc, 0, null };
    }
    operator VkPipelineViewportStateCreateInfo() {
        sc.offset = {0, 0};
        sc.extent = extent;
        return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, null, 0, 1, &viewport, 1, &sc };
    }
    operator VkPipelineRasterizationStateCreateInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, null, 0, VK_FALSE, VK_FALSE,
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE // ... = 0.0f
        };
    }
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 uv;
    ///
    static vec<VkVertexInputAttributeDescription> attrs() {
        return {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, pos)   },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
            { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, uv)    }
        };
    }
};

struct RenderPair {
    VkCommandBuffer   cmd; /// call them this, for ease of understanding of purpose
    UniformBufferData ubo; /// a uniform buffer object is always occupying a cmd of any sort when it comes to rendering
    RenderPair() { }
};

struct PipelineData {
    std::string                name;
    Device                    *device;
    vec<RenderPair>            pairs; /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
    vec<VkDescriptorSet>       desc_sets;
    VkPipelineLayout           layout;
    VkPipeline                 pipeline;
    vec<UniformBufferData>     ubo;
    VBufferData                vbo;      //
    IBufferData                ibo;      //
    bool                       enabled;  //
    map<std::string, Texture>  tx;       // we may need this.
    ///
    operator bool () { return  enabled; }
    bool operator!() { return !enabled; }
    void enable (bool en) { enabled = en; }
    void update(Frame &frame, VkCommandBuffer &rc);
    virtual void bind_descriptions() { }
};

/// we need high level abstraction to get to a vulkanized component model
template <typename U, typename V>
struct Pipeline:PipelineData {
    void bind_descriptions() {
        Device         &device = *this->device;
        auto          bindings = vec<VkDescriptorSetLayoutBinding> {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }};
        VkDescriptorSetLayoutCreateInfo li = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            nullptr, 0, uint32_t(bindings.size()), bindings.data() };
        assert(vkCreateDescriptorSetLayout(pipeline, &li, nullptr, &layout) == VK_SUCCESS);
        ///
        const size_t  n_frames = device.frames.size();
        auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, device); /// add to vec.
        auto                ai = VkDescriptorSetAllocateInfo {};
        ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool      = device;
        ai.descriptorSetCount  = n_frames;
        ai.pSetLayouts         = layouts.data();
        desc_sets.resize(n_frames);
        ///
        if (vkAllocateDescriptorSets(device, &ai, desc_sets.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");
        ///
        size_t i = 0;
        for (auto &f: device.frames) {
            auto &ds = desc_sets[i++];
            auto   w = vec<VkWriteDescriptorSet> {
                f.uniform(ds), f.tx(ds)
            };
            vkUpdateDescriptorSets(device, uint32_t(w.size()), w, 0, nullptr);
        }
    }
    ///
    void create_shaders() {
        Device      &device = *this->device;
        VkShaderModule vert = shader_module(str::format("shaders/{0}.vert.spv", {name}));
        VkShaderModule frag = shader_module(str::format("shaders/{0}.frag.spv", {name}));

        vec<VkPipelineShaderStageCreateInfo> stages {{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_VERTEX_BIT,   vert, name.c_str(), null
        }, {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_FRAGMENT_BIT, frag, name.c_str(), null
        }};

        auto binding = VkVertexInputBindingDescription {
            0, sizeof(V), VK_VERTEX_INPUT_RATE_VERTEX
        };
        auto attrs   = V::attrs();

        VkPipelineVertexInputStateCreateInfo vertex_info {};
        vertex_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_info.vertexBindingDescriptionCount   = 1;
        vertex_info.vertexAttributeDescriptionCount = uint32_t(attrs.size());
        vertex_info.pVertexBindingDescriptions      = &binding;
        vertex_info.pVertexAttributeDescriptions    = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo topology {};
        topology.sType                              = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        topology.topology                           = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        topology.primitiveRestartEnable             = VK_FALSE;
        
        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
        };
        const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0
        };

        /// viewport
        VkViewport viewport = device;
        VkRect2D sc {};
        sc.offset                                   = {0, 0};
        sc.extent                                   = device.extent;

        /// vulkan type data should only be presented on IMAX screens
        VkPipelineViewportStateCreateInfo vs        = device;
        vs.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vs.viewportCount                            = 1;
        vs.pViewports                               = &viewport;
        vs.scissorCount                             = 1;
        vs.pScissors                                = &sc;

        VkPipelineRasterizationStateCreateInfo rs {};
        rs.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.depthClampEnable                         = VK_FALSE;
        rs.rasterizerDiscardEnable                  = VK_FALSE;
        rs.polygonMode                              = VK_POLYGON_MODE_FILL;
        rs.lineWidth                                = 1.0f;
        rs.cullMode                                 = VK_CULL_MODE_BACK_BIT;
        rs.frontFace                                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.depthBiasEnable                          = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo ms {};
        ms.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.sampleShadingEnable                      = VK_FALSE;
        ms.rasterizationSamples                     = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds {};
        ds.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable                          = VK_TRUE;
        ds.depthWriteEnable                         = VK_TRUE;
        ds.depthCompareOp                           = VK_COMPARE_OP_LESS;
        ds.depthBoundsTestEnable                    = VK_FALSE;
        ds.stencilTestEnable                        = VK_FALSE;

        VkPipelineColorBlendAttachmentState cba {};
        cba.colorWriteMask                          = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cba.blendEnable                             = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo blending {};
        blending.sType                              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blending.logicOpEnable                      = VK_FALSE;
        blending.logicOp                            = VK_LOGIC_OP_COPY;
        blending.attachmentCount                    = 1;
        blending.pAttachments                       = &cba;
        blending.blendConstants[0]                  = 0.0f; // likely used differently for different blending methods
        blending.blendConstants[1]                  = 0.0f; // they should be called params not constants; its as 'constant' as a pipeline is
        blending.blendConstants[2]                  = 0.0f;
        blending.blendConstants[3]                  = 0.0f;
        VkPipelineLayoutCreateInfo layout_info      = device;
        ///
        assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout) == VK_SUCCESS);
        ///
        VkGraphicsPipelineCreateInfo pi {};
        pi.sType                                    = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount                               = stages.size();
        pi.pStages                                  = stages.data();
        pi.pVertexInputState                        = &vertex_info;
        pi.pInputAssemblyState                      = &topology;
        pi.pViewportState                           = &vs;
        pi.pRasterizationState                      = &rs;
        pi.pMultisampleState                        = &ms;
        pi.pDepthStencilState                       = &ds;
        pi.pColorBlendState                         = &blending;
        pi.layout                                   = pipeline;
        pi.renderPass                               = pipeline;
        pi.subpass                                  = 0;
        pi.basePipelineHandle                       = VK_NULL_HANDLE;
        ///
        assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
        ///
        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);
    }
    
    /// produce VkShaderModule from source
    VkShaderModule shader_module(path_t path);
    Pipeline(Device *dev, str name);
};

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
