#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
///
#include <dx/dx.hpp>
#include <media/canvas.hpp>
#include <media/obj.hpp>

///
/// a good vulkan api will be fully accessible but minimally interfaced
struct GPU {
protected:
    var              gfx;
    var              present;
    
public:
    enum Capability {
        Present,
        Graphics,
        Complete
    };
    struct Support {
        VkSurfaceCapabilitiesKHR caps;
        vec<VkSurfaceFormatKHR>  formats;
        vec<VkPresentModeKHR>    present_modes;
        bool                     ansio;
    };
    
    Support          support;
    VkSurfaceKHR     surface;
    VkPhysicalDevice gpu;
    VkQueue          queues[2];
    ///
    GPU(VkPhysicalDevice, VkSurfaceKHR);
    uint32_t                index(Capability);
    void                    destroy();
    operator                bool();
    operator                VkPhysicalDevice();
    bool                    operator()(Capability);
    VkSampleCountFlagBits   max_sampling();
};

/// a good vulkan api must be fully accessible but minimally interfaced.
struct Device;
struct Texture;
struct Buffer {
    Device        *device;
    VkDescriptorBufferInfo info;
    VkBuffer       buffer;
    VkDeviceMemory memory;
    void           destroy();
    operator       VkBuffer();
    operator       VkDeviceMemory();
    ///
    Buffer(Device *, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    void copy_to(Buffer &, size_t);
    void copy_to(Texture *);
    inline operator VkDescriptorBufferInfo &() {
        return info;
    }
};

struct IBufferData {
    Buffer          buffer;
    operator VkBuffer() {
        return buffer;
    }
};

struct VBufferData {
    Buffer          buffer;
    operator VkBuffer() {
        return buffer;
    }
};

/// its the stack, thats all.
template <typename V>
struct VertexBuffer:VBufferData {
    VBufferData buffer;
    ///
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(vec<V> &) { }
};

template <typename I>
struct IndexBuffer:VBufferData {
    VBufferData buffer;
    ///
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    IndexBuffer(vec<I> &) { }
};

template <typename U>
struct UniformBuffer {
    Buffer buffer;
    ///
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &buffer
        };
    }
    UniformBuffer(Device *device) {
        buffer = Buffer { device, sizeof(U), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    }
};

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
        VkSampleCountFlagBits   msaa;
        ///
        void destroy();
        void transition_layout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mip_levels);
        static VkImageView create_view(VkDevice, VkImage, VkFormat, VkImageAspectFlags, uint32_t mipLevels);
    
        ///
        /// we need to create the color resources with texture as well, simplify flags if possible
        /// int = mip levels
        int get_mips(int mip_levels);
        Texture(nullptr_t n = null) { }
        Texture(Device *, vec2i, rgba,
                VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, VkSampleCountFlagBits,
                VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Texture(Device *device, Image &im,
                VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, VkSampleCountFlagBits,
                VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Texture(Device *device, vec2i sz, VkImage image, VkImageView view); /// dont use.
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
};

/// What do you call this controller?
struct Frame {
    Device                *device;
    Texture                tx; /// just image and view used; no sampler
    VkFramebuffer          framebuffer;
    UniformBuffer<Uniform> uniform;
    map<str, VkCommandBuffer> render_commands;
    void destroy(VkDevice &device);
    void create(vec<VkImageView> attachments);
    operator VkFramebuffer &() {
        return framebuffer;
    }
};

/// worth having; rid ourselves of more terms =)
struct Descriptor {
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;
    operator VkDescriptorSetLayout &() { return layout; }
    operator VkDescriptorPool      &() { return pool;   }
};

struct PipelineData;
struct Plumbing {
protected:
    Device *device;
    std::map<std::string, PipelineData *> pipelines;
public:
    PipelineData &operator[](std::string n);
    void update(Frame &frame);
};

struct Device {
    void                  create_swapchain();
    void                  create_command_buffers();
    void                  create_render_pass();
    VkImageView           create_iview(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    void                  update();
    
    VkSampleCountFlagBits msaa_samples;
    VkRenderPass          render_pass;
    Descriptor            desc;
    vec<VkCommandBuffer>  command_buffers;
    GPU                  &gpu;
    VkCommandPool         pool;
    VkDevice              device;
    VkQueue               queues[GPU::Complete];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    vec<Frame>            frames;
    Texture               tx_color;
    Texture               tx_depth;
    Plumbing              plumbing;
    
    PipelineData &operator[](std::string n);
    ///
    void            initialize();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    uint32_t        memory_type(uint32_t types, VkMemoryPropertyFlags props);
    
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    operator        VkRenderPass();
    Device(GPU *gpu, bool aa = false, bool val = false);
    VkQueue &operator()(GPU::Capability cap);
    ///
    operator VkViewport &() {
        return viewport;
    }
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

struct PipelineData {
    std::string                name;
    map<std::string, int>      ubo_ids; /// internal.
    Device                    *device;
    vec<VkDescriptorSet>       desc_sets;
    VkPipelineLayout           layout;
    VkPipeline                 pipeline;
    vec<VertexBufferData *>    vx;
    vec<IndexBufferData *>     ix;
    bool                       enabled;
    map<std::string, r32>      f32;
    map<std::string, vec2f>    v2;
    map<std::string, vec3f>    v3;
    map<std::string, vec4f>    v4;
    map<std::string, Texture>  tx;
    ///
    operator bool () { return  enabled;  }
    bool operator!() { return !enabled;  }
    void enable(bool en) { enabled = en; }
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
        auto           layouts = std::vector<VkDescriptorSetLayout> (device.frames.size(), device); /// add to vec.
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
        
        /// indicates a multi-viewport
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
        assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline) == VK_SUCCESS);
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
    
    /// throw things in a tube
    void operator()(Frame &frame, VkCommandBuffer &rc) { ///
        auto        &device = *this->device;
        auto   clear_values = vec<VkClearValue> {
            {        .color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
            { .depthStencil = {1.0f, 0}}};
        auto    render_info = VkRenderPassBeginInfo {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, null, device, frame,
            {{0,0}, device.extent}, uint32_t(clear_values.size()), clear_values };
        vkCmdBeginRenderPass(rc, &render_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(rc, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vec<VkBuffer> buffers = vec<VkBuffer>(vx.size());
            /// iterate through VertexBufferData
            for (auto &v: vx) /// this should be a pair, probably..
                buffers += v;
            VkDeviceSize   offsets[] = {0};
            vkCmdBindVertexBuffers(rc, 0, buffers.size(), buffers, offsets);
        
            /// 32bit indexing seems like a basic default but it should support this param changed through template type I
            /// ibo for creating primitives in reference to vbo
            
            vkCmdBindIndexBuffer(rc, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(rc, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &pipeline.desc_set[i], 0, nullptr);
            vkCmdDrawIndexed(rc, uint32_t(indices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(rc);
    }
    
    /// produce VkShaderModule from source
    VkShaderModule shader_module(path_t path) {
        VkShaderModule m;
        str code = str::read_file(path);
        auto  ci = VkShaderModuleCreateInfo {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, null, 0,
            code.length(), (const uint32_t *)code.cstr()
        };
        assert (vkCreateShaderModule(pipeline, &ci, null, &m) == VK_SUCCESS);
        return m;
    }
    
    /// this can be a method on device as well, pipeline<shader, Vertex>
    Pipeline(Device *dev, str name) : device(device) {
        bind_descriptions();
        create_shaders();
    }
};


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
