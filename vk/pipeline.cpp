#include <vk/vk.hpp>

void PipelineData::Memory::destroy() {
    auto  &device = *this->device;
    vkDestroyDescriptorSetLayout(device, set_layout, null);
    vkDestroyPipeline(device, pipeline, null);
    vkDestroyPipelineLayout(device, pipeline_layout, null);
    for (auto &cmd: frame_commands)
        vkFreeCommandBuffers(device, device.command, 1, &cmd);
}
///
PipelineData::Memory::Memory(std::nullptr_t n) { }

PipelineData::Memory::~Memory() {
    destroy();
}

/// constructor for pipeline memory; worth saying again.
PipelineData::Memory::Memory(Device        &device,  UniformData &ubo,
                             VertexData    &vbo,     IndexData   &ibo,
                             array<Texture *> &tx,   size_t vsize,
                             rgba           clr,     string       shader,
                             VkStateFn      vk_state):
        device(&device), shader(shader), ubo(ubo),
           vbo(vbo), ibo(ibo), tx(tx), vsize(vsize), clr(clr)
{
    /// obtain data via usage
    auto bindings = array<VkDescriptorSetLayoutBinding> {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT,   nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }};
    
    /// create descriptor set layout
    auto desc_layout_info = VkDescriptorSetLayoutCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0, uint32_t(bindings.size()), bindings.data() };
    assert(vkCreateDescriptorSetLayout(device, &desc_layout_info, null, &set_layout) == VK_SUCCESS);
    
    /// of course, you do something else right about here...
    const size_t  n_frames = device.frames.size();
    auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, set_layout); /// add to vec.
    auto                ai = VkDescriptorSetAllocateInfo {};
    ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool      = device.desc.pool;
    ai.descriptorSetCount  = n_frames;
    ai.pSetLayouts         = layouts.data();
    desc_sets.resize(n_frames);
    VkDescriptorSet *ds_data = desc_sets.data();
    VkResult res = vkAllocateDescriptorSets(device, &ai, ds_data);
    assert  (res == VK_SUCCESS);
    ubo.update(&device);
    /// and then its finished.  the descriptor set is created from layouts.  that means something to somebody.  not me.
    
    /// write descriptor sets for all swap image instances
    for (size_t i = 0; i < device.frames.size(); i++) {
        auto &desc_set  = desc_sets[i];
        auto  v_writes  = array<VkWriteDescriptorSet>(size_t(1 + tx.size()));
        
        /// update descriptor sets
        v_writes       += ubo.descriptor(i, desc_set);
        uint32_t i_tx   = 1;
        for (auto t:tx)
            v_writes   += t->descriptor(desc_set, i_tx++);
        
        VkDevice    dev = device;
        size_t       sz = v_writes.size();
        VkWriteDescriptorSet *ptr = v_writes.data();
        vkUpdateDescriptorSets(dev, uint32_t(sz), ptr, 0, nullptr);
    }
    ///
    auto vert = device.module(var::format("shaders/{0}.vert.spv", { shader }), Device::Vertex);
    auto frag = device.module(var::format("shaders/{0}.frag.spv", { shader }), Device::Fragment);
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
    
    /// 
    auto vk_attr            = vbo.fn_attribs();
    auto viewport           = VkViewport(device);
    auto sc                 = VkRect2D {
        .offset             = {0, 0},
        .extent             = device.extent
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
    assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);
    
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
            .rasterizationSamples    = device.sampling,
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
        .renderPass                   = device.render_pass,
        .subpass                      = 0,
        .basePipelineHandle           = VK_NULL_HANDLE
    };
    ///
    if (vk_state)
        vk_state(state);
    
    assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
}

/// create graphic pipeline
void PipelineData::Memory::update(size_t frame_id) {
    auto         &device = *this->device;
    Frame         &frame = device.frames[frame_id];
    VkCommandBuffer &cmd = frame_commands[frame_id];
    
    auto clear_color = [&](rgba c) {
        return VkClearColorValue {{ float(c.r) / 255.0f, float(c.g) / 255.0f,
                                    float(c.b) / 255.0f, float(c.a) / 255.0f }};
    };
    
    /// reallocate command
    vkFreeCommandBuffers(device, device.command, 1, &cmd);
    auto alloc_info = VkCommandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        null, device.command, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    assert(vkAllocateCommandBuffers(device, &alloc_info, &cmd) == VK_SUCCESS);
    
    /// begin command
    auto begin_info = VkCommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    assert(vkBeginCommandBuffer(cmd, &begin_info) == VK_SUCCESS);
    
    ///
    auto   clear_values = array<VkClearValue> {
        {        .color = clear_color(clr)}, // for image rgba  sfloat
        { .depthStencil = {1.0f, 0}}         // for image depth sfloat
    }; /// nothing in canvas showed with depthStencil = 0.0.
    
    auto    render_info = VkRenderPassBeginInfo {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = null,
        .renderPass      = VkRenderPass(device),
        .framebuffer     = VkFramebuffer(frame),
        .renderArea      = {{0,0}, device.extent},
        .clearValueCount = clear_values.size(),
        .pClearValues    = clear_values.data()
    };

    /// gather some rendering ingredients
    vkCmdBeginRenderPass(cmd, &render_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    /// toss together a VBO array
    array<VkBuffer>    a_vbo   = { vbo };
    VkDeviceSize   offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, a_vbo.data(), offsets);
    vkCmdBindIndexBuffer(cmd, ibo, 0, ibo.buffer.type_size == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, desc_sets.data(), 0, nullptr);
    
    /// flip around an IBO and toss it in the oven at 410F
    uint32_t sz_draw = ibo.size();
    vkCmdDrawIndexed(cmd, sz_draw, 1, 0, 0, 0); /// ibo size = number of indices (like a vector)
    vkCmdEndRenderPass(cmd);
    assert(vkEndCommandBuffer(cmd) == VK_SUCCESS);
}
