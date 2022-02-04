#pragma once
#include <dx/dx.hpp>
#include <media/image.hpp>

/// two valid meanings inferred
struct AInternal;

class AI {
protected:
    struct AInternal *i;
public:
    AI(path_t p);
    array<float> operator()(array<var> v);
};
