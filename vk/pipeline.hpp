#pragma once
#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/vk.hpp>
#include <glm/glm.hpp>

struct IVertex {
    virtual vec<VkVertexInputAttributeDescription> attrs() {
        assert(false);
        return {};
    }
    virtual VkVertexInputBindingDescription bind() {
        assert(false);
        return {};
    }
};

struct Vertex:IVertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 uv;

    vec<VkVertexInputAttributeDescription> attrs() {
        return {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, pos)   },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
            { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, uv)    }
        };
    }
    VkVertexInputBindingDescription bind() {
        return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    }
};

struct Shader {
protected:
    // so a graphics Pipeline is a Shader, Vertex Format, Viewport, Primitive style,
    VkShaderModule create_module(path_t path) {
        VkShaderModule result;
        str code = str::read_file(path);
        /// use initializer list for everything *CreateInfo
        auto  ci = VkShaderModuleCreateInfo {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, null, 0,
            code.length, (const uint32_t *)code.cstr()
        };
        assert (vkCreateShaderModule(device, &ci, null, &result) == VK_SUCCESS);
        return result;
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
        assert(vkCreateDescriptorSetLayout(device, &li, nullptr, &descriptorSetLayout) == VK_SUCCESS);
    }
public:
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    
    // you can definitely init a Shader<Vertex, Enum or name>
    Shader(Device &device, str name) {
        VkShaderModule vert = create_module(str::format("shaders/{0}.vert.spv", {name}));
        VkShaderModule frag = create_module(str::format("shaders/{0}.frag.spv", {name}));

        vec<VkPipelineShaderStageCreateInfo> stages {{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_VERTEX_BIT,   vert, name.cstr(), null
        }, {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
            VK_SHADER_STAGE_FRAGMENT_BIT, frag, name.cstr(), null
        }};
        
        auto binding = v.bind();
        auto attrs   = v.attrs();

        VkVertexInputBindingDescription binding_desc = { 0. sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        
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

        /// viewport
        VkViewport viewport {};
        viewport.x                                  = 0.0f;
        viewport.y                                  = 0.0f;
        viewport.width                              = (float) device.extent.width;
        viewport.height                             = (float) device.extent.height;
        viewport.minDepth                           = 0.0f;
        viewport.maxDepth                           = 1.0f;
        
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
        vs.pScissors                                = &scissor;

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
        ms.rasterizationSamples                     = msaaSamples;

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
        blending.blendConstants[0]                  = 0.0f;
        blending.blendConstants[1]                  = 0.0f;
        blending.blendConstants[2]                  = 0.0f;
        blending.blendConstants[3]                  = 0.0f;

        VkPipelineLayoutCreateInfo layout_info {};
        layout_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount                  = 1;
        layout_info.pSetLayouts                     = &descriptorSetLayout;
        
        assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);

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
        pi.layout                                   = pipeline_layout;
        pi.renderPass                               = renderPass;
        pi.subpass                                  = 0;
        pi.basePipelineHandle                       = VK_NULL_HANDLE;

        assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);

        vkDestroyShaderModule(device, frag, nullptr);
        vkDestroyShaderModule(device, vert, nullptr);

        
        
        
        
        
    }

/// a good vulkan api will be fully accessible but minimally interfaced
struct Pipeline {
protected:
    // combine all of this crap since its 1:1 frame ops
    void create();
    Device                    &device;
    VkPipelineLayout           pipeline_layout;
    VkPipeline                 pipeline;
    
    // these are almost certainly 1:1 again
    VkDescriptorSetLayout      descriptor_set_layout;
    VkPipelineLayout           layout;

public:
    /// shaders are created by shader
    void set_shader();
    Pipeline(Device &device);
};

// joystick input would be useful api
