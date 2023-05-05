#pragma once
#include <core/core.hpp>
#ifndef MINIMP3_IMPLEMENTATION
struct mp3dec_ex_t;
#endif

struct audio:mx {
protected:
    struct iaudio *p;
public:
    ptr(audio, mx, iaudio, p);
    audio(path res, bool force_mono = false, int pos = -1, size_t sz = 0);
    array<short>  pcm_data(); 
    int           channels();
    operator          bool();
    bool         operator!();
    size_t            size();
    size_t       mono_size();
};
