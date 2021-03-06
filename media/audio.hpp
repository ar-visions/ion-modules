#include <dx/dx.hpp>

struct Private;
struct Audio {
protected:
    Private *p;
public:
    Audio(std::nullptr_t n);
    Audio();
    Audio(std::filesystem::path, bool force_mono = false, int pos = -1, size_t sz = 0);
   ~Audio();
    size_t       size();
    size_t       mono_size();
    array<short> data();
    int          channels();
    operator     bool();
    bool         operator!();
};
