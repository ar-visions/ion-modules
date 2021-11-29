#pragma once
#include <ux/ux.hpp>

/// nodes can override their own canvas drawing fn, as well as the render fn
/// in this case its all handled by the basic node object; probably shouldn't have instances of just those unless you want to do things meta-programmatically
/// i dont want to actually be forced to speak of such things i can only communicate in code
struct Label:node {
    declare(Label);
};

