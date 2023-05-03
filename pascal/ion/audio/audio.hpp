#pragma once

#ifndef MINIMP3_IMPLEMENTATION
struct mp3dec_ex_t;
#endif

struct iaudio {
    mp3dec_ex_t   *api;
    short         *buf;
    uint64_t       sz;
    ~iaudio() { delete[] buf; }
};

struct audio:mx {
protected:
    inline static bool      init;
    inline static mp3dec_t  dec;
    iaudio &p;

public:
    ctr(audio, mx, iaudio, p);
    audio(path res, bool force_mono = false, int pos = -1, size_t sz = 0);

    /// obtain short waves from skinny aliens
    array<short>  pcm_data(); 
    int           channels();
    operator          bool();
    bool         operator!();
    size_t            size();
    size_t       mono_size();
};
