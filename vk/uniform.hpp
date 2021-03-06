#pragma once

typedef std::function<void(void *)> UniformFn;

struct UniformData {
    struct Memory {
        Device       *dev = null;
        int           struct_sz;
        array<Buffer> buffers;
        UniformFn     fn;
    };
    std::shared_ptr<Memory> m = null;
    
    UniformData(std::nullptr_t n = null) { }
    VkWriteDescriptorSet descriptor(size_t frame_index, VkDescriptorSet &ds);
    void   destroy();
    void   update(Device *dev);
    void   transfer(size_t frame_id);
    inline operator  bool() { return  m && m->dev != null; }
    inline bool operator!() { return !operator bool(); }
    
    /// shouldnt need the serialization
    /*
    UniformData(var &v):
        m(std::shared_ptr<Memory>(new Memory {
            .struct_sz = v["struct_sz"],
            .fn        = v["fn"].convert<UniformFn>()
        })) { }
    ///
    operator var() {
        size_t uni_sz = sizeof(UniformFn);
        std::shared_ptr<uint8_t> b(new uint8_t[uni_sz]);
        memcopy(b.get(), (uint8_t *)&m->fn, uni_sz);
        return Map {
            {"sz", m->struct_sz},
            {"fn", var { Type::ui8, b, uni_sz }}
        };
    }*/
};

template <typename U>
struct UniformBuffer:UniformData { /// will be best to call it 'Uniform', 'Verts', 'Polys'; make sensible.
    UniformBuffer(std::nullptr_t n = null) { }
    UniformBuffer(Device &dev, std::function<void(U &)> fn) {
        m = std::shared_ptr<Memory>(new Memory {
            .dev       = &dev,
            .struct_sz = sizeof(U),
            .fn        = UniformFn([fn](void *u) { fn(*(U *)u); })
        });
    }
};


typedef UniformData Uniform;

