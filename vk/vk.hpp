#pragma once
#include <vulkan/vulkan.hpp>
#include <dx/dx.hpp>
#include <media/canvas.hpp>
#include <vk/gpu.hpp>
#include <vk/device.hpp>
#include <vk/pipeline.hpp> // this one is tricky to organize because of Shader binding
///
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <glm/glm.hpp>
#include <media/obj.hpp>


///
/// a good vulkan api must be fully accessible but minimally interfaced.
/// simplifying the API with casts is the real showcase here.  i honestly
/// believe anyone who reads this will walk away thinking 'why on earth
/// did i consider this a difficult api to learn?' when you realize how
/// many types fold into simple abstract.  i do believe it should be
/// coded in the way done on vk-app for anything in production.
/// to manage code, you need to be able to read it.  and humans only read
/// so much at once.  packing in as much on the screen as possible is just
/// how you can drive understanding easier.  these operations need to be
/// seen to be understood.

/// a good vulkan api will be fully accessible but minimally interfaced
struct Device;
struct Buffer {
    VkDescriptorBufferInfo info;
    VkBuffer       buffer;
    VkDeviceMemory memory;
    ///
    void           destroy();
    operator       VkBuffer();
    operator       VkDeviceMemory();
    
    Buffer(Device *, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    void copy_to(Buffer &, size_t);
    
    inline operator VkDescriptorBufferInfo &() {
        return info;
    }
};

template <typename V>
struct VertexBuffer {
    Buffer          buffer;
    VertexBuffer(vec<V> &) {
    }
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

/// cant use Texture in frame?
struct Frame {
    VkImage                 image;
    VkImageView             view;
    VkFramebuffer           framebuffer;
    UniformBuffer<Uniform>  ubuffer;
    void destroy(VkDevice &device);
};

/// worth having; rid ourselves of more terms =)
struct Descriptor {
    VkDescriptorSetLayout layout;
    VkDescriptorPool      pool;
    operator VkDescriptorSetLayout &() {
        return layout;
    }
};

struct Device {
    /// combine all of this since its 1:1 frame ops
    void        create_swapchain();
    void        create_command_buffers();
    void        create_render_pass();
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    /// we want to pslit off as much as we can to 'pipeline', including shader for now
    
    VkSampleCountFlagBits      msaa_samples;
    VkRenderPass               render_pass;
    Descriptor                 desc;
    vec<VkCommandBuffer>       command_buffers;
    
    /// throw it all in the gumbo. -- 7th grade teacher

    GPU                       &gpu;
    VkCommandPool              pool;
    VkDevice                   device;
    VkQueue                    queues[GPU::Complete];
    
    /// important that these are here until it makes complete to split them off
    VkSwapchainKHR             swap_chain;
    VkFormat                   format;
    VkExtent2D                 extent;
    
    /// combined image, view framebuffer, and uniform buffer into a singular data structure
    vec<Frame>                 frames;
    
    ///
    void            initialize();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    operator        VkRenderPass();
    Device(GPU *gpu, bool aa = false, std::vector<const char*> *validation = null);
    VkQueue &operator()(GPU::Capability cap);
    
    operator VkViewport() {
        return { 0.0f, 0.0f, r32(extent.width), r32(extent.height),
                 0.0f, 1.0f };
    }
    
    operator VkDescriptorSetLayout &() {
        return desc;
    }
    
};

///
struct Texture {
    protected:
    VkSampler create_sampler();
    void create_resources(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits numSamples,
                          VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory, VkSampler *sampler);
    
    public:
    Device        *device;
    VkImage        image      = VK_NULL_HANDLE;
    VkImageView    view       = VK_NULL_HANDLE;
    VkDeviceMemory memory     = VK_NULL_HANDLE;
    VkSampler      sampler    = VK_NULL_HANDLE;
    VkFormat       format;
    vec2i          sz         = { 0, 0 };
    int            mip_levels = 0;
    ///
    void           destroy();
    operator       VkImageView();
    
    void transition_layout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mip_levels);

    Texture(vec2i size, rgba clr, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mip_levels = -1);
    Texture(Image &im, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, int mip_levels = -1);
    
    operator VkDescriptorImageInfo() {
        return { sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }
    operator  bool();
    bool operator!();
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
    static VkVertexInputBindingDescription bind() {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }
};

template <typename U, typename V>
struct Pipeline {
protected:
    map<std::string, int>      ubo_ids; // internal.
    Device                    *device;
    vec<VkDescriptorSet>       desc_sets;
    VkDescriptorSetLayout      desc_layout;
    VkPipelineLayout           pipeline_layout;
    VkPipeline                 pipeline;
    

public:
    map<std::string, r32>      f32;
    map<std::string, vec2f>    v2;
    map<std::string, vec3f>    v3;
    map<std::string, vec4f>    v4;
    map<std::string, Texture>  tx; // this is great.
    
    void create_descriptor_sets() {
        Device         &device = *this->device;
        const size_t  n_frames = device.frames.size();
        auto           layouts = std::vector<VkDescriptorSetLayout> (device.frames.size(), *device);
        auto                ai = VkDescriptorSetAllocateInfo {};
        ///
        ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool      = *device;
        ai.descriptorSetCount  = n_frames;
        ai.pSetLayouts         = layouts;
        desc_sets.resize(n_frames);
        /// created once for each pipeline, destroyed with the pipeline
        if (vkAllocateDescriptorSets(device, &ai, desc_sets.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");
        ///
        for (auto &f: frames) {
            auto desc_wri = vec<VkWriteDescriptorSet> {
                device.uniform(f),
                device.sampler(f)
            };
            vkUpdateDescriptorSets(device, uint32_t(dw.size()), desc_wri, 0, nullptr);
        }
    }
    
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
        Device      &device = *dev;
        VkShaderModule vert = shader_module(str::format("shaders/{0}.vert.spv", {name}));
        VkShaderModule frag = shader_module(str::format("shaders/{0}.frag.spv", {name}));

        vec<VkPipelineShaderStageCreateInfo> stages {{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_VERTEX_BIT,   vert, name.cstr(), null
        }, {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_FRAGMENT_BIT, frag, name.cstr(), null
        }};
        
        auto binding = V::bind();
        auto attrs   = V::attrs();

        VkVertexInputBindingDescription binding_desc = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        
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
        
        /// #
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
        VkPipelineViewportStateCreateInfo vs {};
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

        VkPipelineLayoutCreateInfo layout_info {};
        layout_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount                  = 1;
        layout_info.pSetLayouts                     = &desc_layout;
        
        /// lol. this is silly
        assert(vkCreatePipelineLayout(pipeline, &pipeline, nullptr, &pipeline) == VK_SUCCESS);

        ///
        /// Pipeline <Shader, Vertex>(device; Enum for DisplayType)
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

        assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);

        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);
    }
    
    void create_descriptor_set_layout() {
        auto bindings = vec<VkDescriptorSetLayoutBinding> {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
        };
        VkDescriptorSetLayoutCreateInfo li = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            nullptr, 0, uint32_t(bindings.size()), bindings.data()
        };
        assert(vkCreateDescriptorSetLayout(pipeline, &li, nullptr, &layout) == VK_SUCCESS);
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
