#pragma once
#include <vk/vk.hpp>
#include <vk/opaque.hpp>

/// worth having; rid ourselves of more terms =)
struct Descriptor {
    Device               *dev;
    sh<VkDescriptorPool>  pool;
    
    /// in ge: nein! konstruktor steht immer oben
    Descriptor(Device *dev = null);
    void          destroy();
    Descriptor &operator=(const Descriptor &d);
    
    /// open up the methods.
    /// this is more Olson-like than my usual and also its open-ended.
    /// this is a must.  i feel it.
    /// its lexical across the component graph, so its like C and you can override.
    
};
