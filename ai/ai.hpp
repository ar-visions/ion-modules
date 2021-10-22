#pragma once
#include <data/data.hpp>
#include <media/image.hpp>

struct AInternal;

class AI {
protected:
    struct AInternal *i;
public:
    AI(path_t p);
    vec<float> operator()(vec<var> v);
};
