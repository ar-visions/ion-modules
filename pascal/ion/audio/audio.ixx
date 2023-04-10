module;

#include <new>

template <typename T>
void inplace(T *ptr) {
        (ptr) -> ~T();
    new (ptr)     T();
}


#define  MINIMP3_ONLY_MP3
#define  MINIMP3_ONLY_SIMD
//#define  MINIMP3_FLOAT_OUTPUT
#define  MINIMP3_IMPLEMENTATION

#include "minimp3_ex.h"

#include <core/core.hpp>

export module audio;
import core;

struct iaudio {
    mp3dec_ex_t    api;
    short         *buf;
    uint64_t       sz;
    ~iaudio() { delete[] buf; }
};

export {

struct abc_data {
    int test;
};


struct audio:mx {
protected:
    inline static bool      init;
    inline static mp3dec_t  dec;
    iaudio &p;

public:
    ctr(audio, mx, iaudio, p);
    audio(path res, bool force_mono = false, int pos = -1, size_t sz = 0) : audio() {
        if (!init) {
            mp3dec_init(&dec);
            init = true;
        }
        
        /// open mp3, allocate and read selected pcm data from decoder
        mp3dec_ex_t *a = &p.api;
        assert(              !mp3dec_ex_open(a, res.cs(), MP3D_SEEK_TO_SAMPLE));
        assert( pos == -1 || !mp3dec_ex_seek(a, uint64_t(pos)));
        p.sz = pos == -1 ? a->samples : uint64_t(sz);
        assert(p.sz > 0);
        p.buf = new short[p.sz];
        assert(mp3dec_ex_read(a, p.buf, a->samples) == p.sz);
        
        /// convert to mono if we feel like it.
        const int   channels = p.api.info.channels;
        if (channels > 1 && force_mono) {
            size_t      n_sz = p.sz / channels;
            short     *n_buf = new short[n_sz];
            for (int       i = 0; i < p.sz; i += channels) {
                float    sum = 0;
                for (int   c = 0; c < channels; c++)
                        sum += p.buf[i + c];
                n_buf[i / channels] = sum / channels;
            }
            delete[] p.buf;
            p.buf = n_buf;
            p.sz  = n_sz;
            p.api.info.channels = 1; // hope you dont mind buddy
        }
    }

    /// obtain short waves from skinny aliens
    array<short> pcm_data() {
        array<short> res(size_t(p.sz));
        for (int i = 0; i < p.sz; i++)
            res += p.buf[i];
        return res;
    }

    int           channels() { return  p.api.info.channels; }
    operator          bool() { return  p.sz; }
    bool         operator!() { return !p.sz; }
    size_t            size() { return  p.sz; }
    size_t       mono_size() { return  p.sz / p.api.info.channels; }
};
}



