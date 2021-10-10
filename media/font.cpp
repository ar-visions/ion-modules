#include <data/data.hpp>
#include <media/font.hpp>
#include <core/SkImage.h>
#include <core/SkFont.h>
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkSurface.h>
#include <core/SkFontMgr.h>
#include <core/SkFontMetrics.h>

typedef std::map<std::string, sk_sp<SkTypeface>> Typefaces;

static sk_sp<SkFontMgr>    sk_manager;
static vec<Font *>         cache;
static Typefaces           faces;

struct FontData {
    sk_sp<SkTypeface>      face;
    SkFont                 font;
    std::filesystem::path  path;
};

Font::Font() { }

Font::Font(std::string alias, double sz, std::filesystem::path path, FontData *data)
         : alias(alias), sz(sz), path(path), data(data) { }

Font::Font(std::string alias, double sz, std::filesystem::path path) {
    if (!sk_manager) {
         sk_manager = SkFontMgr::RefDefault();
         assert(sk_manager);
    }
    Font *font = null;
    if (!std::filesystem::is_empty(path)) {
        assert(sz > 0);
        std::string p = path.string();
        sk_sp<SkTypeface> face;
        if (faces.count(p) == 0) {
            faces[p] = sk_manager->makeFromFile(p.c_str(), 0);
            assert(faces[p]);
        } else
            face = faces[p];
        font = new Font {
            alias, sz, path, new FontData { face, SkFont(faces[p], sz), path }};
        cache += font;
    } else {
        for (Font *f: cache)
            if (f->alias == alias && (sz == 0 || f->sz == sz)) {
                if (sz > 0) {
                    font = f;
                    break;
                }
                if (!font || (font->sz > f->sz))
                     font = f;
            }
    }
    assert(font);
    this->alias = font->alias;
    this->sz    = font->sz;
    this->data  = font->data;
}

