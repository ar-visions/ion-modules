#pragma once

struct Device;
struct Texture;
struct iBuffer;
struct iTexture;

struct Buffer {
    sh<iBuffer> intern;
    ///
    Buffer(std::nullptr_t n = null);

    /// impl: generic
    Buffer(Device *, void *, Type, Size, VkBufferUsageFlags, VkMemoryPropertyFlags);
    
    /// intf: pointer
    template <typename T>
    Buffer(Device *dev, T *dat, Size sz, VkBufferUsageFlags &u, VkMemoryPropertyFlags &p):
        Buffer(dev, Id<T>(), dat, sz, u, p) { }

    /// intf: array
    template <typename T>
    Buffer(Device *dev, array<T> &v, VkBufferUsageFlags &u, VkMemoryPropertyFlags &p):
        Buffer(dev, Id<T>(), v.data(), v.size(), u, p) { }
    ///
    Size  total_bytes();
    Size  size();
    Type  type();
    void  destroy    ();
    void  allocate   (VkBufferUsageFlags &u);
    void  transfer   (void *v);
    void  copy_to    (Buffer &b, size_t s);
    void  copy_to    (iTexture *t);
    ///
    Buffer &operator=(const Buffer &);
    ///
    operator               VkBuffer &();
    operator         VkDeviceMemory &();
    operator VkDescriptorBufferInfo &();
};


