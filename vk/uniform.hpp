#pragma once

typedef std::function<void(void *)> UniformFn;

struct UniformData {
    struct Memory {
        Device     *device = null;
        int         id;
        int         struct_sz;
        vec<Buffer> buffers;
        UniformFn   fn;
    };
    std::shared_ptr<Memory> m = null;
    
    UniformData(nullptr_t n = null) { }
    VkWriteDescriptorSet write_desc(size_t frame_index, VkDescriptorSet &ds);
    void   destroy();
    void   update(Device *device);
    void   transfer(size_t frame_id);
    inline operator  bool() { return  m && m->device != null; }
    inline bool operator!() { return !operator bool(); }
    ///
    UniformData(var &v):
        m(std::shared_ptr<Memory>(new Memory {
            .id        = v["id"],
            .struct_sz = v["struct_sz"],
            .fn        = v["fn"].convert<UniformFn>()
        })) { }
    ///
    operator var() {
        size_t uni_sz = sizeof(UniformFn);
        std::shared_ptr<uint8_t> b(new uint8_t[uni_sz]);
        memcopy(b.get(), (uint8_t *)&m->fn, uni_sz);
        return Args {
            {"id", m->id},
            {"sz", m->struct_sz},
            {"fn", var { var::ui8, b, uni_sz }}
        };
    }
};

template <typename U>
struct UniformBuffer:UniformData { /// will be best to call it 'Uniform', 'Verts', 'Polys'; make sensible.
    UniformBuffer(nullptr_t n = null) { }
    UniformBuffer(Device &device, int id, std::function<void(U &)> fn) {
        m = std::shared_ptr<Memory>(new Memory {
            .device    = &device,
            .id        = id,
            .struct_sz = sizeof(U),
            .fn        = UniformFn([fn](void *u) { fn(*(U *)u); })
        });
    }
};


typedef UniformData Uniform;

