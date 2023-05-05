#define  MINIMP3_ONLY_MP3
#define  MINIMP3_ONLY_SIMD
//#define  MINIMP3_FLOAT_OUTPUT
#define  MINIMP3_IMPLEMENTATION

#include "minimp3_ex.h"

#include <core/core.hpp>
#include <audio/audio.hpp>

struct iaudio {
    bool           init;
    mp3dec_t       dec;
    mp3dec_ex_t    api;
    short         *buf;
    u64            sz;
    ~iaudio() { delete[] buf; }
};

audio::audio(path res, bool force_mono, int pos, size_t sz) : audio() {
    if (!p->init) {
        mp3dec_init(&p->dec);
        p->init = true;
    }
    
    /// open mp3, allocate and read selected pcm data from decoder
    assert(              !mp3dec_ex_open(&p->api, res.cs(), MP3D_SEEK_TO_SAMPLE));
    assert( pos == -1 || !mp3dec_ex_seek(&p->api, uint64_t(pos)));
    p->sz = pos == -1 ? p->api.samples : uint64_t(sz);
    assert(p->sz > 0);
    p->buf = new short[p->sz];
    assert(mp3dec_ex_read(&p->api, p->buf, p->api.samples) == p->sz);
    
    /// convert to mono if we feel like it.
    const int   channels = p->api.info.channels;
    if (channels > 1 && force_mono) {
        size_t      n_sz = p->sz / channels;
        short     *n_buf = new short[n_sz];
        for (int       i = 0; i < p->sz; i += channels) {
            float    sum = 0;
            for (int   c = 0; c < channels; c++)
                    sum += p->buf[i + c];
            n_buf[i / channels] = sum / channels;
        }
        delete[] p->buf; /// delete previous data
        p->buf = n_buf; /// set new data 
        p->sz  = n_sz; /// set new size
        p->api.info.channels = 1; /// set to 1 channel
    }
}

/// obtain short waves from skinny aliens
array<short> audio::pcm_data() {
    array<short> res(size_t(p->sz));
    for (int i = 0; i < p->sz; i++)
        res += p->buf[i];
    return res;
}

int           audio::channels() { return  p->api.info.channels; }
audio::operator          bool() { return  p->sz; }
bool         audio::operator!() { return !p->sz; }
size_t            audio::size() { return  p->sz; }
size_t       audio::mono_size() { return  p->sz / p->api.info.channels; }
