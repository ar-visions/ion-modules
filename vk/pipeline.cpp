#include <vk/vk.hpp>



vec<VkVertexInputAttributeDescription> Vertex::attrs() {
    auto attrs = vec<VkVertexInputAttributeDescription> {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, pos)   },
        { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) },
        { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex, uv)    }
    };
    return attrs;
}

void PipelineData::destroy() {
    vkDestroyDescriptorSetLayout(*device, set_layout, null);
}

void PipelineData::initialize()        {
    auto  &device = *this->device;
    auto bindings = vec<VkDescriptorSetLayoutBinding> {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr },
            /// these are the same binding as whats given from Texture Sampler
        
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }};
            /// we need any and all image registration done at the pipeline
            /// constructor or perhaps template given as an array param
    
    VkDescriptorSetLayoutCreateInfo li = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0, uint32_t(bindings.size()), bindings.data() };
    
    assert(vkCreateDescriptorSetLayout(device, &li, nullptr, &set_layout) == VK_SUCCESS);
    
    /// fairly certain we need to host this on Pipeline...
    ///
    const size_t  n_frames = device.frames.size();
    auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, set_layout); /// add to vec.
    auto                ai = VkDescriptorSetAllocateInfo {};
    ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool      = device.desc.pool;
    ai.descriptorSetCount  = n_frames;
    ai.pSetLayouts         = layouts.data(); /// the struct is set_layout, the data is a set_layout (1) broadcast across the vector
    desc_sets.resize(n_frames);
    ///
    assert (vkAllocateDescriptorSets(device, &ai, desc_sets.data()) == VK_SUCCESS);
    ///
    ///
    size_t i = 0;
    for (auto &f: device.frames) {
        auto &ds = desc_sets[i];
        auto &uu = f.attachments[Frame::Color]; /// need to figure out if we need have separate uniform space per swap frame
        auto   w = vec<VkWriteDescriptorSet> { ubo.write_desc(i, ds), uu(ds) }; // we are to use OUR uniform.  we delete this frame uni thingy now.
        vkUpdateDescriptorSets(device, uint32_t(w.size()), w, 0, nullptr);
        i++;
    }

    auto vert = device.module(str::format("shaders/{0}.vert.spv", {name}), Device::Vertex);
    auto frag = device.module(str::format("shaders/{0}.frag.spv", {name}), Device::Fragment);
    vec<VkPipelineShaderStageCreateInfo> stages {{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_VERTEX_BIT,   vert, name.c_str(), null
    }, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_FRAGMENT_BIT, frag, name.c_str(), null
    }};
    auto binding = VkVertexInputBindingDescription {
        0, uint32_t(vsize), VK_VERTEX_INPUT_RATE_VERTEX
    };

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
    ms.rasterizationSamples                     = device.sampling;

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
    VkPipelineLayoutCreateInfo layout_info      = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts    = &set_layout
    };
    ///
    assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);
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
    pi.layout                                   = pipeline_layout;
    pi.renderPass                               = device.render_pass;
    pi.subpass                                  = 0;
    pi.basePipelineHandle                       = VK_NULL_HANDLE;
    ///
    assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
}
