#pragma once

struct UniformData {
    Device     *device;
    vec<Buffer> buffers;
    void       *data; /// its good to fix the address memory, update it as part of the renderer, if the user goes away the UniformBuffer will as well.
    
    UniformData(): device(null), data(null) { }
    VkWriteDescriptorSet write_desc(size_t frame_index, VkDescriptorSet &ds);
    void destroy();
    void transfer();
    void initialize(size_t sz);
};

template <typename U>
struct UniformBuffer:UniformData { /// will be best to call it 'Uniforms'
    UniformBuffer(nullptr_t n = null) {
        device = null;
        data   = null;
    }
    UniformBuffer(Device &device, U &data) {
        this->device = &device;
        this->data   = &data;
        initialize(sizeof(U));
    }
};
