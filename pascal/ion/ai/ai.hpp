#pragma once

/// two valid meanings inferred
struct AInternal;

class AI:mx {
protected:
    AInternal *i;
public:
    ptr(AI, mx, AInternal, i);
    AI(path_t p);
    array<float> operator()(array<var> v);
};
