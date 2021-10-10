#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <list>
#include <vulkan/vulkan.h>

#define  SK_VULKAN
#include <core/SkImage.h>
#include <gpu/vk/GrVkBackendContext.h>
#include <gpu/GrBackendSurface.h>
#include <gpu/GrDirectContext.h>
#include <gpu/gl/GrGLInterface.h>
#include <core/SkFont.h>
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkSurface.h>
#include <core/SkFontMgr.h>
#include <core/SkFontMetrics.h>
#include <effects/SkGradientShader.h>
#include <effects/SkImageFilters.h>
#include <effects/SkDashPathEffect.h>

#include <media/canvas.hpp>
#include <media/image.hpp>
#include <media/color.hpp>
#include <data/data.hpp>
#include <media/canvas.hpp>
#include <data/data.hpp>
#include <data/vec.hpp>
#include <vk/vk.hpp>

inline SkColor sk_color(rgba c) {
    const double s = 255.0;
    return SkColor(uint32_t(c.b * s)        | (uint32_t(c.g * s) << 8) |
                  (uint32_t(c.r * s) << 16) | (uint32_t(c.a * s) << 24));
}

struct Skia {
    sk_sp<GrDirectContext> sk_context;
    Skia(sk_sp<GrDirectContext> sk_context) : sk_context(sk_context) { }
    
    static Skia *Context() {
        static struct Skia *sk = null;
        if (sk) return sk;

        //GrBackendFormat gr_conv = GrBackendFormat::MakeVk(VK_FORMAT_R8G8B8_SRGB);
        sk_sp<GrDirectContext> sk_context;
        
        #if defined(SK_VULKAN)
        GrVkBackendContext grc {
            Vulkan::get_instance(),
            Vulkan::get_gpu(),
            Vulkan::get_device(),
            Vulkan::get_queue(),
            Vulkan::get_queue_index(),
            Vulkan::get_version()
        };
        grc.fMaxAPIVersion = Vulkan::get_version();
        //grc.fVkExtensions = new GrVkExtensions(); // internal needs population perhaps
        grc.fGetProc = [](const char *name, VkInstance inst, VkDevice dev) -> PFN_vkVoidFunction {
            return (dev == VK_NULL_HANDLE) ? vkGetInstanceProcAddr(inst, name) :
                                             vkGetDeviceProcAddr  (dev,  name);
        };
        sk_context = GrDirectContext::MakeVulkan(grc);
        #elif defined(SK_GL)
        sk_context = GrDirectContext::MakeGL(GrGLMakeNativeInterface());
        #elif defined(SK_METAL)
        #endif
        assert(sk_context);
        sk = new Skia(sk_context);
        return sk;
    }
};

struct Context2D:ICanvasBackend {
    sk_sp<SkSurface> sk_surf = null;
    SkCanvas      *sk_canvas = null;
    vec2i                 sz = { 0, 0 };
    VkImage         vk_image = null;
    
    struct State {
        SkPaint ps;
    };
    
    // the cost, of voids.. yes.
    void *copy_bstate(void *bs) {
        if (!bs)
            return null;
        State *s = (State *)bs;
        return new State { s->ps };
    }
    
    Context2D(vec2i sz) {
        // get canvas working without framebuffer, it wont use that
        // the idea of this subsystem is it fits into a larger drawing system
        // it wont be a hinderance on performance to go from
        // VkImage to our dest VkSwapbuffer or whatever its called.
        
      //Vulkan::init();
      //Vulkan::resize(sz.x, sz.y);
        
        GrDirectContext *ctx    = Skia::Context()->sk_context.get();
        auto imi                = GrVkImageInfo { };
        imi.fImage              = Vulkan::get_image();
        imi.fImageTiling        = VK_IMAGE_TILING_OPTIMAL;
        imi.fImageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imi.fFormat             = VK_FORMAT_R8G8B8A8_UNORM;
      //imi.fImageUsageFlags    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imi.fSampleCount        = 1;
        imi.fLevelCount         = 1;
        imi.fCurrentQueueFamily = Vulkan::get_queue_index(); //VK_QUEUE_FAMILY_IGNORED;
        imi.fProtected          = GrProtected::kNo;
        imi.fSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        
        vk_image = imi.fImage;
        auto rt                 = GrBackendRenderTarget { sz.x, sz.y, imi };
        sk_surf                 = SkSurface::MakeFromBackendRenderTarget(
                                       ctx, rt, kBottomLeft_GrSurfaceOrigin,
                                       kRGBA_8888_SkColorType, null, null);
        sk_canvas               = sk_surf->getCanvas();
        set_size(sz);
    }
    
    void flush(DrawState &) {
        sk_canvas->flush();
    }
    
    void save(DrawState &d_state) {
        State *s    = new State;
        State *prev = (State *)(d_state.b_state);
        if (prev) {
            s->ps = prev->ps;
            delete  prev;
        } else {
            s->ps = SkPaint { };
        }
        d_state.b_state = s;
    }

    void restore(DrawState &d_state) {
        delete (State *)d_state.b_state;
    }
    
    void *data() {
        return null; // todo: SkImageSnapshot
    }

    void set_size(vec2i sz_new) {
        sz = sz_new;
    }

    void text(DrawState &state, str &s, rectd &rect, Vec2<Align> &align, vec2 &offset) {
    }

    void clip(DrawState &st, rectd &path) {
        assert(false);
    }

    void stroke(DrawState &state, rectd &rect) {
    }

    void fill(DrawState &state, rectd &rect) {
        State   *s = (State *)state.b_state; // backend state (this)
        SkPaint ps = SkPaint(s->ps);
        if (state.opacity != 1.0f)
            ps.setAlpha(float(ps.getAlpha()) * float(state.opacity));
        SkRect   r = SkRect {
            SkScalar(rect.x),          SkScalar(rect.y),
            SkScalar(rect.x + rect.w), SkScalar(rect.y + rect.h) };
        sk_canvas->drawRect(r, ps);
    }
    
    void gaussian(DrawState &state, vec2 &sz, rectd crop) {
        SkImageFilters::CropRect crect = { };
        if (crop) {
            SkRect rect = { SkScalar(crop.x),
                            SkScalar(crop.y),
                            SkScalar(crop.x + crop.w),
                            SkScalar(crop.y + crop.h) };
            crect       = SkImageFilters::CropRect(rect);
        }
        sk_sp<SkImageFilter> filter = SkImageFilters::Blur(sz.x, sz.y, nullptr, crect);
        State *st = (State *)host->state.b_state;
        st->ps.setImageFilter(std::move(filter));
    }
};

void DrawState::copy(const DrawState &r) {
    host       = r.host;
    stroke_sz  = r.stroke_sz;
    font_scale = r.font_scale;
    opacity    = r.opacity;
    m          = r.m;
    color      = r.color;
    clip       = r.clip;
    blur       = r.blur;
    b_state    = r.host->copy_bstate(r.b_state);
}


struct Terminal:ICanvasBackend {
    static map<str, str> t_text_colors_8;
    static map<str, str> t_bg_colors_8;
    const char *reset = "\u001b[0m";
    vec2i sz = { 0, 0 };
    vec<ColoredGlyph> glyphs;
    
    str ansi_color(rgba c, bool text) {
        map<str, str> &map = text ? t_text_colors_8 : t_bg_colors_8;
        if (c.a < 32)
            return "";
        str hex    = str("#") + str(c);
        str result = map.count(hex) ? map[hex] : "";
        return result;
    }

    Terminal(vec2i sz) {
        set_size(sz);
    }

    void *data() {
        return &glyphs;
    }

    void set_size(vec2i sz_new) {
        glyphs = null;
        sz  = sz_new;
        if (sz.x > 0 && sz.y > 0) {
            glyphs = vec<ColoredGlyph>(sz.y * sz.x);
            glyphs.fill(null);
        }
    }

    void set_char(DrawState &st, int x, int y, ColoredGlyph cg) {
        if (x < 0 || x >= sz.x) return;
        if (y < 0 || y >= sz.y) return;
        assert(!st.clip || st.clip.rect);
        size_t index  = y * sz.x + x;
        if (cg.border) glyphs[index].border  = cg.border;
        if (cg.s)  {
            str &gc = glyphs[index].s;
            if ((gc == "+") || (gc == "|" && cg.s == "-") ||
                               (gc == "-" && cg.s == "|"))
                glyphs[index].s = "+";
            else
                glyphs[index].s = cg.s;
        }
        if (cg.fg) glyphs[index].fg = cg.fg;
        if (cg.bg) glyphs[index].bg = cg.bg;
    }

    // gets the effective character, including bordering
    str get_char(DrawState &st, int x, int y) {
        size_t ix = std::clamp(x, 0, sz.x - 1);
        size_t iy = std::clamp(y, 0, sz.y - 1);
        str blank = " ";
        
        auto get_str = [&]() -> str {
            auto value_at = [&](int x, int y) -> ColoredGlyph * {
                if (x < 0 || x >= sz.x) return null;
                if (y < 0 || y >= sz.y) return null;
                return &glyphs[y * sz.x + x];
            };
            
            auto is_border = [&](int x, int y) {
                ColoredGlyph *cg = value_at(x, y);
                if (cg && cg->border > 0) {
                    int test = 0;
                    test++;
                }
                return cg ? (cg->border > 0) : false;
            };
            
            ColoredGlyph &cg = glyphs[iy * sz.x + ix];
            return cg.s;
            
            auto t  = is_border(x, y - 1);
            auto b  = is_border(x, y + 1);
            auto l  = is_border(x - 1, y);
            auto r  = is_border(x + 1, y);
            auto q  = is_border(x, y);
            
            if (q) {
                if (t && b && l && r) return "\u254B"; //  +
                if (t && b && r)      return "\u2523"; //  |-
                if (t && b && l)      return "\u252B"; // -|
                if (l && r && t)      return "\u253B"; // _|_
                if (l && r && b)      return "\u2533"; //  T
                if (l && t)           return "\u251B";
                if (r && t)           return "\u2517";
                if (l && b)           return "\u2513";
                if (r && b)           return "\u250F";
                if (t && b)           return "\u2503";
                if (l || r)           return "\u2501";
            }
            return cg.s;
        };
        
        return get_str();
    }

    void text(DrawState &state, str &s, rectd &rect, Vec2<Align> &align, vec2 &offset) {
        assert(glyphs.size() == (sz.x * sz.y));
        size_t len = s.length();
        if (!len)
            return;
        int x0 = std::clamp(int(std::round(rect.x)), 0, sz.x - 1);
        int x1 = std::clamp(int(std::round(rect.x + rect.w)), 0, sz.x - 1);
        int y0 = std::clamp(int(std::round(rect.y)), 0, sz.y - 1);
        int y1 = std::clamp(int(std::round(rect.y + rect.h)), 0, sz.y - 1);
        int  w = (x1 - x0) + 1;
        int  h = (y1 - y0) + 1;
        if (!w || !h) return;
        int tx = align.x == Align::Start  ? (x0) :
                 align.x == Align::Middle ? (x0 + (x1 - x0) / 2) - int(std::ceil(len / 2.0)) :
                 align.x == Align::End    ? (x1 - len) : (x0);
        int ty = align.y == Align::Start  ? (y0) :
                 align.y == Align::Middle ? (y0 + (y1 - y0) / 2) - int(std::ceil(len / 2.0)) :
                 align.y == Align::End    ? (y1 - len) : (y0);
        tx = std::clamp(tx, x0, x1);
        ty = std::clamp(ty, y0, y1);
        size_t ix = std::clamp(tx, 0, sz.x - 1);
        size_t iy = std::clamp(ty, 0, sz.y - 1);
        size_t len_w = std::min(x1 - tx, int(len));
        for (size_t i = 0; i < len_w; i++) {
            int x = ix + i + offset.x;
            int y = iy + 0 + offset.y;
            if (x < 0 || x >= sz.x || y < 0 || y >= sz.y)
                continue;
            std::string ch = s.substr(i, 1);
            set_char(state, tx + i, ty, {0, ch, null, state.color});
        }
    }

    void clip(DrawState &st, rectd &path) {
        assert(false); // todo: clipping in terminal
        // since theres no notion of 'unclip', it must be a bit odd how skia works
        // it should be sufficient to have a singular path stored for clipping
        // any additional clip just boolean &'s this rect
    }

    void stroke(DrawState &state, rectd &rect) {
        assert(rect);
        recti r = rect;
        int  ss = std::min(2.0, std::round(state.stroke_sz));

        ColoredGlyph cg_0 = {ss, "-", null, state.color};
        ColoredGlyph cg_1 = {ss, "|", null, state.color};
        
        for (size_t ix = 0; ix < r.w; ix++) {
            set_char(state, r.x + ix, r.y,               cg_0);
            if (r.h > 1)
                set_char(state, r.x + ix, r.y + r.h - 1, cg_0);
        }
        for (size_t iy = 0; iy < r.h; iy++) {
            set_char(state, r.x, r.y + iy,               cg_1);
            if (r.w > 1)
                set_char(state, r.x + r.w - 1, r.y + iy, cg_1);
        }
    }

    void fill(DrawState &state, rectd &rect) {
        if (!state.color)
            return;
        recti r = rect;
        str t_color = ansi_color(state.color, false);
        int x0 = std::max(0, r.x),
            x1 = std::min(sz.x - 1, r.x + r.w);
        int y0 = std::max(0, r.y),
            y1 = std::min(sz.y - 1, r.y + r.h);
        ColoredGlyph cg = {0, " ", state.color, null};
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                set_char(state, x, y, cg);
    }
};

map<str, str> Terminal::t_text_colors_8 = {
    { "#000000" , "\u001b[30m" },
    { "#ff0000",  "\u001b[31m" },
    { "#00ff00",  "\u001b[32m" },
    { "#ffff00",  "\u001b[33m" },
    { "#0000ff",  "\u001b[34m" },
    { "#ff00ff",  "\u001b[35m" },
    { "#00ffff",  "\u001b[36m" },
    { "#ffffff",  "\u001b[37m" }};

map<str, str> Terminal::t_bg_colors_8 = {
    { "#000000" , "\u001b[40m" },
    { "#ff0000",  "\u001b[41m" },
    { "#00ff00",  "\u001b[42m" },
    { "#ffff00",  "\u001b[43m" },
    { "#0000ff",  "\u001b[44m" },
    { "#ff00ff",  "\u001b[45m" },
    { "#00ffff",  "\u001b[46m" },
    { "#ffffff",  "\u001b[47m" }};


void ICanvasBackend::translate(DrawState &st, vec2  &tr)  { st.m     = st.m * m44::translation({tr.x, tr.y, 0.0}); }
void ICanvasBackend::    scale(DrawState &st, vec2  &sc)  { st.m     = st.m * m44::scale(      {sc.x, sc.y, 1.0}); }
void ICanvasBackend::   rotate(DrawState &st, double deg) { st.m     = st.m * m44::rotation_z(deg); }
void ICanvasBackend::    color(DrawState &st, rgba  &c)   { st.color = c;  }
void ICanvasBackend:: gaussian(DrawState &st, vec2  &sz, rectd cr)  { st.blur  = sz; }

Canvas::Canvas(nullptr_t n) : type(Undefined) { }

Canvas::Canvas(vec2i sz, Canvas::Type type) : type(type),
               backend(type == Terminal ? (ICanvasBackend *)new ::Terminal(sz) :
                                          (ICanvasBackend *)new ::Context2D(sz)) {
    backend->host = this;
    defaults();
    save();
}

void *Canvas::data() { return backend ? ((::Terminal *)backend)->data() : null; }
str  Canvas::get_char(  int x, int y     ) { return backend->get_char(state, x, y); }
void Canvas::defaults(                   ) { state = {this, 1, 1, 1, m44::identity(), "#000f", vec2 {0, 0}}; }
void Canvas::stroke_sz( double sz        ) { state.stroke_sz = sz;                                  }
void Canvas::font_scale(double sc        ) { state.font_scale  = sc;                                }
void Canvas::save(                       ) { states.push(state); backend->save(state);              }
void Canvas::restore(                    ) {
    state = states.top();
    states.pop();
    assert(states.size() >= 1);
}
void Canvas::scale(     vec2   sc        ) { state.m = state.m * m44::scale({sc.x, sc.y, 1});       }
void Canvas::rotate(    double d         ) { state.m = state.m * m44::rotation_z(d);                }
str  Canvas::ansi_color(rgba c, bool t   ) { return backend->ansi_color(c, t);                      }
void Canvas::translate( vec2   tr        ) { backend->translate(state, tr);                         }
void Canvas::color(     rgba   c         ) { backend->color(state, c);                              }
void Canvas::gaussian(  vec2   s, rectd c) { backend->gaussian(state, s, c);                        }
void Canvas::clip(      Path   &path     ) { backend->clip  (state, path);                          }
void Canvas::clip(      rectd  &rect     ) { backend->clip  (state, rect);                          }
void Canvas::fill(      Path   &path     ) { backend->fill  (state, path);                          }
void Canvas::fill(      rectd  &rect     ) { backend->fill  (state, rect);                          }
void Canvas::stroke(    Path   &path     ) { backend->stroke(state, path);                          }
void Canvas::stroke(    rectd  &rect     ) { backend->stroke(state, rect);                          }
void Canvas::flush()                       { backend->flush (state);                                }
bool Canvas::operator==(Type   t)          { return type == t;                                      }
void Canvas::text(str s,rectd r, Vec2<Align> align, vec2 o) {
    backend->text(state, s, r, align, o);
}

void *Canvas::copy_bstate(void *bs) {
    return backend->copy_bstate(bs);
}

Path::Path()                    { }
Path::Path(rectd r) : rect(r)   { }
Path::Path(const Path &ref)     { copy((Path &)ref); }
Path::operator rectd()          { return rect;       }
Path &Path::operator=(Path ref) {
    if (this != &ref)
        copy(ref);
    return *this;
}

void Path::copy(Path &ref) {
    a = ref.a;
    rect = ref.rect;
}

Path &Path::move(vec2 v) {
    a += Points {{ v }};
    return *this;
}

Path &Path::line(vec2 v) {
    if (a.size() == 0)
        a += Points {{ v }};
    else
        a.back() += v;
    return *this;
}

Path &Path::rectangle(rectd r, vec2 rounded, bool left, bool top, bool right, bool bottom) {
    assert(a.size() == 0);
    if (rounded.x == 0 && rounded.y == 0 && left && top && right && bottom)
        rect = r;
    else {
        assert(false);
    }
    return *this;
}

Path &Path::arc(vec2 center, double radius, double rads_from, double rads, bool move_start) {
    return *this;
}

Path &Path::bezier(vec2 cp0, vec2 cp1, vec2 p) {
    return *this;
}

Path &Path::quad(vec2 q0, vec2 q1) {
    return *this;
}

Path Path::offset(double o) {
    return Path(*this);
}

vec2 Path::xy() {
    if (rect)
        return rect.xy();
    assert(a.size());
    assert(a[0].size());
    return a[0][0];
}

Path::operator bool() {
    return a.size() || rect;
}

bool Path::operator!() {
    return !(operator bool());
}
