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
#include <core/SkPathMeasure.h>
#include <core/SkTextBlob.h>
#include <effects/SkGradientShader.h>
#include <effects/SkImageFilters.h>
#include <effects/SkDashPathEffect.h>

#include <media/canvas.hpp>
#include <media/font.hpp>
#include <media/image.hpp>
#include <media/color.hpp>
#include <dx/dx.hpp>
#include <dx/vec.hpp>
#include <vk/vk.hpp>

inline SkColor sk_color(rgba c) {
    auto sk = SkColor(uint32_t(c.b)        | (uint32_t(c.g) << 8) |
                     (uint32_t(c.r) << 16) | (uint32_t(c.a) << 24));
    return sk;
}

struct Skia {
    sk_sp<GrDirectContext> sk_context;
    Skia(sk_sp<GrDirectContext> sk_context) : sk_context(sk_context) { }
    
    static Skia *Context() {
        static struct Skia *sk = null;
        if (sk) return sk;

        //GrBackendFormat gr_conv = GrBackendFormat::MakeVk(VK_FORMAT_R8G8B8_SRGB);
        sk_sp<GrDirectContext> sk_context;
        Vulkan::init();
        
        #if defined(SK_VULKAN)
            GrVkBackendContext grc {
                Vulkan::instance(),
                Vulkan::gpu(),
                Vulkan::device(),
                Vulkan::queue(),
                Vulkan::queue_index(),
                Vulkan::version()
            };
            grc.fMaxAPIVersion = Vulkan::version();
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
    Texture               tx = null;
    
    struct State {
        SkPaint ps;
    };
    
    void *copy_bstate(void *bs) {
        if (!bs)
            return null;
        State *s = (State *)bs;
        return new State { s->ps };
    }

    Context2D(vec2i sz) {
        tx                      = Vulkan::texture(sz);
        GrDirectContext *ctx    = Skia::Context()->sk_context.get();
        auto imi                = GrVkImageInfo { };
        imi.fImage              = VkImage(tx);
        imi.fImageTiling        = VK_IMAGE_TILING_OPTIMAL;
        imi.fImageLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
        imi.fFormat             = VK_FORMAT_R8G8B8A8_UNORM;
     ///imi.fImageUsageFlags    = VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;//VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // i dont think so.
        imi.fSampleCount        = 1;
        imi.fLevelCount         = 1;
        imi.fCurrentQueueFamily = Vulkan::queue_index(); //VK_QUEUE_FAMILY_IGNORED;
        imi.fProtected          = GrProtected::kNo;
        imi.fSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        vk_image                = imi.fImage;
        auto rt                 = GrBackendRenderTarget { sz.x, sz.y, imi };
        sk_surf                 = SkSurface::MakeFromBackendRenderTarget(ctx, rt,
                                      kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, null, null);
        sk_canvas               = sk_surf->getCanvas();
        set_size(sz);
    }
    
    void font(DrawState &ds, Font &f) { }
    void save(DrawState &ds) {
        State *s    = new State;
        State *prev = (State *)(ds.b_state);
        if (prev) {
            s->ps = prev->ps;
            delete  prev;
        } else {
            s->ps = SkPaint { };
        }
        ds.b_state = s;
        sk_canvas->save();
    }
    
    void    clear(DrawState &ds) { sk_canvas->clear(sk_color(ds.color)); }
    void    clear(DrawState &ds, rgba &c) { sk_canvas->clear(sk_color(c)); }
    void    flush(DrawState &ds) { sk_canvas->flush(); }
    void  restore(DrawState &ds) {
        delete (State *)ds.b_state;
        sk_canvas->restore();
    }
    void    *data()              { return null; }
    void set_size(vec2i sz_new)  { sz = sz_new; }
    vec2i    size()              { return sz; }
    
    /// console would just think of everything in char units. like it is.
    /// measuring text would just be its length, line height 1.
    
    TextMetrics measure(DrawState &ds, str &text) {
        SkFontMetrics mx;
        SkFont     &font = *ds.font.handle();
        auto         adv = font.measureText(text.cstr(), text.len(), SkTextEncoding::kUTF8);
        auto          lh = font.getMetrics(&mx);
        return TextMetrics {
            .w           = adv,
            .h           = abs(mx.fAscent) + abs(mx.fDescent),
            .ascent      = mx.fAscent,
            .descent     = mx.fDescent,
            .line_height = lh,
            .cap_height  = mx.fCapHeight
        };
    }

    /// the text out has a rect, controls line height, scrolling offset and all of that nonsense we need to handle
    /// as a generic its good to have the rect and alignment enums given.  there simply isnt a user that doesnt benefit
    /// it effectively knocks out several redundancies to allow some components to be consolidated with style difference alone

    str ellipsis(DrawState &ds, str &text, rectd &rect, TextMetrics &tm) {
        const str el = "...";
        str       cur, *p = &text;
        int       trim = p->len();
        tm             = measure(ds, (str &)el);
        
        if (tm.w >= rect.w)
            trim = 0;
        else
            for (;;) {
                tm = measure(ds, *p);
                if (tm.w <= rect.w || trim == 0)
                    break;
                if (tm.w > rect.w && trim >= 1) {
                    cur = text.substr(0, --trim) + el;
                    p   = &cur;
                }
            }
        return (trim == 0) ? "" : (p == &text) ? text : cur;
    }
    
    /// the lines are most definitely just text() calls, it should be up to the user to perform multiline
    void text(DrawState &ds, str &text, rectd &rect, Vec2<Align> &align, vec2 &offset, bool ellip) {
        State   *s = (State *)ds.b_state; // backend state (this)
        SkPaint ps = SkPaint(s->ps);
        ps.setColor(sk_color(ds.color));
        if (ds.opacity != 1.0f)
            ps.setAlpha(float(ps.getAlpha()) * float(ds.opacity));
        SkFont  &f = *ds.font.handle();
        vec2   pos = { 0, 0 };
        str  stext;
        str *ptext = &text;
        TextMetrics tm;
        if (ellip) {
            stext  = ellipsis(ds, text, rect, tm);
            ptext  = &stext;
        } else
            tm     = measure(ds, *ptext);
        ///
        auto    tb = SkTextBlob::MakeFromText(ptext->cstr(), ptext->len(), (const SkFont &)f, SkTextEncoding::kUTF8);
        ///
        pos.x = (align.x == Align::End)    ? rect.x + rect.w     - tm.w :
                (align.x == Align::Middle) ? rect.x + rect.w / 2 - tm.w / 2 :
                                             rect.x;
        pos.y = (align.y == Align::End)    ? rect.y + rect.h     - tm.h :
                (align.y == Align::Middle) ? rect.y + rect.h / 2 - tm.h / 2 - ((-tm.descent + tm.ascent) / 1.66) :
                                             rect.y;
        sk_canvas->drawTextBlob(
            tb, SkScalar(pos.x + offset.x),
                SkScalar(pos.y + offset.y), ps);
    }

    void clip(DrawState &ds, rectd &path) {
        assert(false);
    }

    void stroke(DrawState &ds, rectd &rect) {
    }
    
    void cap(DrawState &ds, Cap::Type c) {
        State *s = (State *)ds.b_state;
        ///
        s->ps.setStrokeCap(c == Cap::Blunt ? SkPaint::kSquare_Cap :
                           c == Cap::Round ? SkPaint::kRound_Cap  :
                                             SkPaint::kButt_Cap);
    }
    
    void join(DrawState &ds, Join::Type j) {
        State *s = (State *)ds.b_state;
        ///
        s->ps.setStrokeJoin(j == Join::Bevel ? SkPaint::kBevel_Join :
                            j == Join::Round ? SkPaint::kRound_Join  :
                                               SkPaint::kMiter_Join);
    }
    
    // we are to put everything in path.
    void fill(DrawState &ds, Path &path) {
        State   *s = (State *)ds.b_state;
        SkPaint ps = SkPaint(s->ps);
        ///
        ps.setAntiAlias(!path.rect);
        ps.setColor(sk_color(ds.color));

        ///
        if (ds.opacity != 1.0f)
            ps.setAlpha(float(ps.getAlpha()) * float(ds.opacity));
        
        /// path checks if there is an offset, keeps cache
        sk_canvas->drawPath(*path.sk_path(&ps), ps);
    }
    
    void fill(DrawState &ds, rectd &rect) {
        State   *s = (State *)ds.b_state; // backend state (this)
        SkPaint ps = SkPaint(s->ps);
        ///
        ps.setColor(sk_color(ds.color));
        ///
        if (ds.opacity != 1.0f)
            ps.setAlpha(float(ps.getAlpha()) * float(ds.opacity));
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
    font       = r.font;
    font_scale = r.font_scale;
    opacity    = r.opacity;
    m          = r.m;
    color      = r.color;
    clip       = r.clip;
    blur       = r.blur;
    b_state    = r.host ? r.host->copy_bstate(r.b_state) : null;
}

/// this is a very KiSS-oriented Terminal Canvas
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
    
    vec2i size() {
        return sz;
    }

    Terminal(vec2i sz) {
        set_size(sz);
    }

    void *data() {
        return &glyphs;
    }

    void set_size(vec2i sz_new) {
        sz     = sz_new;
        glyphs = vec<ColoredGlyph>(sz.y * sz.x, null);
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

    void text(DrawState &state, str &s, rectd &rect, Vec2<Align> &align, vec2 &offset, bool ellip) {
        assert(glyphs.size() == (sz.x * sz.y));
        size_t len = s.len();
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
        assert(false);
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


void ICanvasBackend::    clear(DrawState &st)             { }
void ICanvasBackend::  texture(DrawState &st, Image *im)  { st.im = im; }
void ICanvasBackend::     font(DrawState &st, Font  &f)   { st.font = f; }
void ICanvasBackend::translate(DrawState &st, vec2  &tr)  { st.m = st.m.translate(tr); }
void ICanvasBackend::    scale(DrawState &st, vec2  &sc)  { st.m = st.m.scale(sc); }
void ICanvasBackend::   rotate(DrawState &st, double deg) { st.m = st.m.rotate_z(deg); }
void ICanvasBackend::    color(DrawState &st, rgba  &c)   { st.color = c;  }
void ICanvasBackend:: gaussian(DrawState &st, vec2  &sz, rectd cr)  { st.blur  = sz; }

Canvas::Canvas(nullptr_t n) : type(Undefined) { }

/// transition to shared ptr for backend
Canvas::Canvas(vec2i sz, Canvas::Type type, var *result) : type(type),
               backend(type == Terminal ? (ICanvasBackend *)new ::Terminal(sz) :
                                          (ICanvasBackend *)new ::Context2D(sz)) {
    backend->host   = this;
    backend->result = result;
    default_font    = Font::default_font();
    defaults();
    save();
}

void  Canvas::restore() {
     state = states.top();
     states.pop();
     /// needs a backend function: todo.
     assert(states.size() >= 1);
}

void *Canvas::data()                    { return type ? ((::Terminal *)backend)->data() : null; }
str   Canvas::get_char(int x, int y)    { return backend->get_char(state, x, y);           }
TextMetrics Canvas::measure(str s)      { return backend->measure(state, s);               }
vec2i Canvas::size()                    { return backend->size();                          }
void *Canvas::copy_bstate(void *bs)     { return type ? backend->copy_bstate(bs) : null;   }
void  Canvas::save()                    { states.push(state); backend->save(state);        }
void  Canvas::defaults()                { state = { this, 1, 1, 1, m44(), "#000f", vec2 {0, 0}, default_font }; }
void  Canvas::stroke_sz( double sz)     { state.stroke_sz = sz;                            }
void  Canvas::font(Font &f)             { state.font = f;                                  }
void  Canvas::font_scale(double sc)     { state.font_scale  = sc;                          }
void  Canvas::scale(vec2 sc)            { state.m = state.m.scale(sc);                     }
void  Canvas::rotate(double d)          { state.m = state.m.rotate_z(d);                   }
void  Canvas::translate(vec2 tr)        { backend->translate(state, tr);                   }
void  Canvas::texture(Image &im)        { backend->texture  (state, im ? &im : null);      }
void  Canvas::color(rgba c)             { backend->color    (state, c);                    }
void  Canvas::gaussian(vec2 s, rectd c) { backend->gaussian (state, s, c);                 }
void  Canvas::clip(Path &path)          { backend->clip     (state, path);                 }
void  Canvas::clip(rectd &rect)         { backend->clip     (state, rect);                 }
void  Canvas::fill(Path &path)          { backend->fill     (state, path);                 }
void  Canvas::fill(rectd &rect)         { backend->fill     (state, rect);                 }
void  Canvas::stroke(Path &path)        { backend->stroke   (state, path);                 }
void  Canvas::stroke(rectd &rect)       { backend->stroke   (state, rect);                 }
void  Canvas::clear()                   { backend->clear    (state);                       }
void  Canvas::clear(rgba c)             { backend->clear    (state, c);                    }
void  Canvas::flush()                   { backend->flush    (state);                       }
void  Canvas::cap(Cap::Type t)          { backend->cap      (state, t);                    }
void  Canvas::join(Join::Type t)        { backend->join     (state, t);                    }

bool  Canvas::operator==(Type   t)       { return type == t;                               }
str   Canvas::ansi_color(rgba c, bool t) { return backend->ansi_color(c, t);               }
void  Canvas::text(str text, rectd rect, Vec2<Align> align, vec2 offset, bool ellipsis) {
    backend->text(state, text, rect, align, offset, ellipsis);
}

Path::Path()                    { }
Path::Path(rectd r) : rect(r)   { }
Path::Path(const Path &ref)     { copy((Path &)ref); }
Path::operator rectd &()        { return rect;       }
Path &Path::operator=(Path ref) {
    if (this != &ref)
        copy(ref);
    return *this;
}

void Path::copy(Path &ref) {
    if (ref.p)
        p = new SkPath(*ref.p);
    rect = ref.rect;
}

Path &Path::rectangle(rectd r, vec2 radius, bool r_tl, bool r_tr, bool r_br, bool r_bl) {
    if ((radius.x == 0 && radius.y == 0) || !(r_tl || r_tr || r_br || r_bl))
        rect = r;
    else {
        vec2 tl = r.xy();
        vec2 tr = vec2(tl.x + r.w, tl.y);
        vec2 br = vec2(tl.x + r.w, tl.y + r.h);
        vec2 bl = vec2(tl.x, tl.y + r.h);
        return rect_v4(
            vec4(tl.x, tl.y, r_tl ? radius.x : 0, r_tl ? radius.y : 0),
            vec4(tr.x, tr.y, r_tr ? radius.x : 0, r_tr ? radius.y : 0),
            vec4(br.x, br.y, r_br ? radius.x : 0, r_br ? radius.y : 0),
            vec4(bl.x, bl.y, r_bl ? radius.x : 0, r_bl ? radius.y : 0));
    }
    return *this;
}

Path::~Path() {
    if (offsets) {
        for (int i = 0; i < max_offsets; i++)
            delete offsets[i];
        free(offsets);
    }
    delete p;
}

Path &Path::move(vec2 v) {
    if (!p) p = new SkPath();
    p->moveTo ( v.x,  v.y);
    return *this;
}

Path &Path::line(vec2 v) {
    if (!p) p = new SkPath();
    p->lineTo ( v.x,  v.y);
    return *this;
}

Path &Path::bezier(vec2 p0, vec2 p1, vec2 v) {
    if (!p) p = new SkPath();
    p->cubicTo(p0.x, p0.y, p1.x, p1.y, v.x, v.y);
    return *this;
}

Path &Path::quad(vec2 q0, vec2 q1) {
    if (!p) p = new SkPath();
    p->quadTo(q0.x, q0.y, q1.x, q1.y);
    return *this;
}

/// using degrees, everywhere...
Path &Path::arc(vec2 c, double r, double d_from, double d, bool m) {
    if (!p) p = new SkPath();
    if (d  > 360.0)
        d -= 360.0 * std::floor(d / 360.0);
    if (d >= 360.0)
        d  =   0.0;
    auto rect = SkRect { SkScalar(c.x - r), SkScalar(c.y - r),
                         SkScalar(c.x + r), SkScalar(c.y + r) };
    p->arcTo(rect, d_from, d, m);
    return *this;
}

Path &Path::rect_v4(vec4 tl, vec4 tr, vec4 br, vec4 bl) {
    /// --------------------------
    /// tl_p tl_x            tr_x tr_p
    /// tl_y                      tr_y
    /// bl_y                      br_y
    /// bl_p bl_x            br_x br_p
    /// --------------------------
    vec2 p_tl  = tl.xy();
    vec2 v_tl  = tr.xy() - p_tl;
    real l_tl  = sqrt(v_tl.xs() + v_tl.ys());
    vec2 d_tl  = v_tl / l_tl;
    /// ---------------------
    vec2 p_tr  = tr.xy();
    vec2 v_tr  = br.xy() - p_tr;
    real l_tr  = sqrt(v_tr.xs() + v_tr.ys());
    vec2 d_tr  = v_tr / l_tr;
    ///
    vec2 p_br  = br.xy();
    vec2 v_br  = bl.xy() - p_br;
    real l_br  = sqrt(v_br.xs() + v_br.ys());
    vec2 d_br  = v_br / l_br;
    ///
    vec2 p_bl  = bl.xy();
    vec2 v_bl  = tl.xy() - p_bl;
    real l_bl  = sqrt(v_bl.xs() + v_bl.ys());
    vec2 d_bl  = v_bl / l_bl;
    ///
    vec2 r_tl  = { std::min(tl.w, l_tl / 2.0), std::min(tl.z, l_bl / 2.0) };
    vec2 r_tr  = { std::min(tr.w, l_tr / 2.0), std::min(tr.z, l_br / 2.0) };
    vec2 r_br  = { std::min(br.w, l_br / 2.0), std::min(br.z, l_tr / 2.0) };
    vec2 r_bl  = { std::min(bl.w, l_bl / 2.0), std::min(bl.z, l_tl / 2.0) };
    
    /// pos +/- [ dir * scale ]
    vec2 tl_x = p_tl + d_tl * r_tl;
    vec2 tl_y = p_tl - d_bl * r_tl;
    ///
    vec2 tr_x = p_tr - d_tl * r_tr;
    vec2 tr_y = p_tr + d_tr * r_tr;
    ///
    vec2 br_x = p_br + d_br * r_br;
    vec2 br_y = p_br + d_bl * r_br;
    ///
    vec2 bl_x = p_bl - d_br * r_bl;
    vec2 bl_y = p_bl - d_tr * r_bl;
    ///
    vec2 cp0, cp1;
    move(tl_x);
    line(tr_x);
    ///
    bezier((p_tr + tr_x) / 2.0,
           (p_tr + tr_y) / 2.0, tr_y);
    line(br_y);
    ///
    bezier((p_br + br_y) / 2.0,
           (p_br + br_x) / 2.0, br_x);
    line(bl_x);
    ///
    bezier((p_bl + bl_x) / 2.0,
           (p_bl + bl_y) / 2.0, bl_y);
    line(tl_y);
    ///
    bezier((p_tl + tl_y) / 2.0,
           (p_tl + tl_x) / 2.0, tl_x);
    ///
    return *this;
}

/// the offset is read only, less we read it back from skia (data is opaque i believe)
void Path::set_offset(real o) {
    this->o = o;
}

bool Path::contains(vec2 v) {
    return p ? p->contains(v.x, v.y) : rect.contains(v);
}

Path::operator bool() {
    return p || rect;
}

bool Path::operator!() {
    return !(operator bool());
}

rectd &Path::bounds() {
    if (!rect) {
        SkRect r = p->getBounds(); /// invalidate rect on updates
        rect.x = r.fLeft;
        rect.y = r.fTop;
        rect.w = r.fRight  - r.fLeft;
        rect.h = r.fBottom - r.fTop;
    }
    return rect;
}

real Path::w()      { return bounds().w; };
real Path::h()      { return bounds().h; };
real Path::aspect() { return bounds().h / bounds().w; };
vec2 Path::xy()     {
    bounds();
    return rect.xy();
}

/// dont mind the params
SkPath *Path::sk_path(SkPaint *paint) {
    if (!std::isnan(o) && o != 0) {
        assert(p);
        if (!offsets)
             offsets = (Path **)calloc(max_offsets, sizeof(Path *));
        Path *output = null;
        for (int   i = 0; i < max_offsets; i++) {
            Path *offset = offsets[i];
            if (offset && offset->o_cache == o) {
                output = offset;
                break;
            }
        }
        if (!output) {
            const int l     = max_offsets - 1;
            delete offsets[l];
            offsets[l]      = new Path();
            output          = offsets[l];
            output->p       = new SkPath(*p);
            output->o_cache = o;
            //SkPath *u_path = output->p;
            
            ///
            SkPath  fpath;
            SkPaint cp = SkPaint(*paint);
            cp.setStyle(SkPaint::kStroke_Style);
            cp.setStrokeWidth(std::abs(o) * 2);
            cp.setStrokeJoin(SkPaint::kRound_Join);
            cp.getFillPath((const SkPath &)*p, &fpath);
            /// todo: the negative offset needs a path selection
            /// unfortunately the path verbs and path points dont line up in skia
            /// 
            if (o < 0) {
                output->p->reverseAddPath(fpath);
                output->p->setFillType(SkPathFillType::kWinding );
                /*
                skia has an incomplete api around path management and sub sections.  its quite incomplete.
                the following code doesnt line up the te amount of points so i dont know how to stride through the ops */
                //------------
                //SkPathMeasure pm = { *output->p, false, 1.0 };
                /*
                SkPath                        *u_path = output->p;
                SkSTArray<16, GrGLubyte, true> verbs;
                SkSTArray<16, SkPoint, true>   points;
                int n_verbs  = u_path->countVerbs();
                int n_points = u_path->countPoints();
                verbs.resize_back (n_verbs);
                points.resize_back(n_points);
                u_path->getPoints (&points[0], n_points);
                u_path->getVerbs  ( &verbs[0], n_verbs);
                
                int ip = 0;
                for (int i = 0; i < n_verbs; i++) {
                    enum SkPath::Verb vb = (enum SkPath::Verb)verbs[i];
                    switch (vb) {
                    case SkPath::kMove_Verb: // 1
                        ip += 1;
                        break;
                    case SkPath::kLine_Verb: // 1
                        ip += 2;
                        break;
                    case SkPath::kCubic_Verb: // 3
                        ip += 3;
                        break;
                    case SkPath::kQuad_Verb: // 4
                        ip += 4;
                        break;
                    case SkPath::kClose_Verb: // 0
                        ip += 0;
                        break;
                    default:
                        assert(false);
                        break;
                    }
                }
                assert(n_points == ip);
                */
            } else
                output->p->addPath(fpath);

            //output->p->toggleInverseFillType();
            ///
        }
        return output->p;
    }
    assert(p);
    return p;
}
