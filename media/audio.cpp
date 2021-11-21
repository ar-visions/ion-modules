#include <media/audio.hpp>
#define  MINIMP3_ONLY_MP3
#define  MINIMP3_ONLY_SIMD
//#define  MINIMP3_FLOAT_OUTPUT
#define  MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

struct Private {
    mp3dec_ex_t    api;
    short         *buf; /// get shorty. thats funny
    uint64_t       sz;
};

static bool        init;
static mp3dec_t    dec;

Audio::Audio(nullptr_t n) { }

Audio::Audio() {
    if (!init) {
        mp3dec_init(&dec);
        init = true;
    }
    p = new Private;
}

size_t Audio::size() {
    return p->sz;
}

size_t Audio::mono_size() {
    return p->sz / p->api.info.channels;
}

Audio::operator bool()  { return  p; }
bool Audio::operator!() { return !p; }

Audio::Audio(std::filesystem::path path, bool force_mono, int pos, size_t sz) : Audio() {
    ///
    /// open mp3, allocate and read selected pcm data from decoder
    mp3dec_ex_t *a = &p->api;
    assert(              !mp3dec_ex_open(a, path.string().c_str(), MP3D_SEEK_TO_SAMPLE));
    assert( pos == -1 || !mp3dec_ex_seek(a, uint64_t(pos)));
    p->sz = pos == -1 ? a->samples : uint64_t(sz);
    assert(p->sz > 0);
    p->buf = new short[p->sz];
    assert(mp3dec_ex_read(a, p->buf, a->samples) == p->sz);
    ///
    /// convert to mono if we feel like it.
    const int   channels = p->api.info.channels;
    if (channels > 1 && force_mono) {
        size_t      n_sz = p->sz / channels;
        short     *n_buf = new short[n_sz];
        for (int       i = 0; i < p->sz; i += channels) {
            float    sum = 0;
            for (int   c = 0; c < channels; c++)
                    sum += p->buf[i + c];
            int    index = i / channels;
            n_buf[index] = sum / channels;
        }
        delete[] p->buf;
        p->buf = n_buf;
        p->sz  = n_sz;
        p->api.info.channels = 1; // hope you dont mind buddy
    }
}

int Audio::channels() {
    return p->api.info.channels;
}

std::vector<short> Audio::data() {
    auto res = std::vector<short>();
    res.reserve(p->sz);
    for (int i = 0; i < p->sz; i++)
        res.push_back(p->buf[i]);
    return res;
}

Audio::~Audio() {
    delete[] p->buf;
    delete p;
}
