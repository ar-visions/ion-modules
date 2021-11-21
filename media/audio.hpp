#include <data/data.hpp>

struct Private;
struct Audio {
protected:
    Private *p;
public:
    Audio(nullptr_t n);
    Audio();
    Audio(std::filesystem::path, bool force_mono = false, int pos = -1, size_t sz = 0);
   ~Audio();
    size_t              size();
    size_t              mono_size();
    std::vector<short>  data();
    int                 channels();
    operator bool();
    bool     operator!();
};
