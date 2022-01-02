#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>
#include <media/font.hpp>
#include <media/color.hpp>
#include <media/image.hpp>
#include <dx/m44.hpp>

typedef vec<vec2> Points;

///
struct Canvas;
struct Align:io {
    enum Type {
        Undef,
        Start,
        Middle,
        End
    } type;
               Align(Type t = Undef) : type(t) { }
    void        copy(const Align &ref) { type = ref.type;        }
    void importer(var &data)           { type = Type(int(data)); }
    var  exporter()                    { return int(type);       }
    bool  operator==(Type t)           { return type == t;       }
    io_define(Align, type != Undef);
};

typedef Vec2<Align> AlignV2;

struct Filter {
    enum Type {
        None,
        Blur,
        Brightness,
        Contrast,
        DropShadow,
        Grayscale,
        HueRotate,
        Invert,
        Opacity,
        Saturate,
        Sepia
    };
    enum Type type;
    float     params[16];

    Filter(Type t = None) {
        type = t;
        memclear(params);
    }
    Filter(nullptr_t n) : Filter(None) { }
    operator bool()     { return type != None; }
    bool operator!()    { return type == None; }
};

struct TextMetrics {
    real            w = 0,
                    h = 0;
    real       ascent = 0,
              descent = 0;
    real  line_height = 0,
           cap_height = 0;
};

struct Blending:io {
    enum Type {
        Clear,
        Src,        Dst,
        SrcOver,    DstOver,
        SrcIn,      DstIn,
        SrcOut,     DstOut,
        SrcATop,    DstATop,
        Xor, Plus,  Modulate,
        Screen,     Overlay,
        Darken,     Lighten,
        ColorDodge, ColorBurn,
        HardLight,  SoftLight,
        Difference, Exclusion,
        Multiply,   Hue,
        Saturation, Color
    } type = Type(-1);
    
    Blending() { }
    Blending(Type type) : type(type) { }
    
    /// data import export
    void copy(const Blending &ref) { type = ref.type;         }
    var exporter()                 { return int32_t(type);    }
    void importer(var &d)          { type = Type(int32_t(d)); }
    
    io_define(Blending, type >= Clear);
};

struct Path {
    vec<Points>   a;
    rectd         rect;
               Path();
               Path(std::initializer_list<Points> v) {
                   for (auto &points:v)
                       a += points;
               }
               Path(rectd r);
               Path(const Path &ref);
    Path &operator=(Path ref);
    void       copy(Path &ref);
    Path      &move(vec2 v);
    Path      &line(vec2 v);
    Path    &bezier(vec2 cp0, vec2 cp1, vec2 p);
    Path      &quad(vec2 q0, vec2 q1);
    Path &rectangle(rectd r, vec2 rounded = {0,0}, bool left = true,
                    bool top = true, bool right = true, bool bottom = true);
    Path       &arc(vec2 c, double radius, double rads_from, double rads, bool move_start);
    Path     offset(double o);
    operator  rectd &();
    vec2         xy();
    real          w() { return rect.w; };
    real          h() { return rect.h; };
    real     aspect() { return rect.h / rect.w; };
    operator  bool();
    bool operator!();
};

struct ColoredGlyph {
    int  border;
    str  s;
    rgba bg;
    rgba fg;
    ColoredGlyph(nullptr_t n) : s(""), bg(null), fg(null) { }
    ColoredGlyph(int border, str s, rgba bg, rgba fg) : border(border), s(s), bg(bg), fg(fg) { }
    str ansi();
    bool operator==(ColoredGlyph &lhs) {
        return   (border == lhs.border) && (s == lhs.s) &&
                 (bg == lhs.bg) && (fg == lhs.fg);
    }
    bool operator!=(ColoredGlyph &lhs) {
        return !((border == lhs.border) && (s == lhs.s) &&
                 (bg == lhs.bg) && (fg == lhs.fg));
    }
};

struct DrawState {
    Canvas *host;
    Image  *im    = null;
    double stroke_sz;
    double font_scale;
    double opacity;
    m44    m;
    rgba   color;
    Path   clip;
    vec2   blur;
    Font   font;
    void  *b_state = null;
    
           DrawState(Canvas *h, double s, double f, double o, m44 m, rgba c, vec2 b, Font font) :
                     host(h), stroke_sz(s), font_scale(f), opacity(o),
                     m(m), color(c), blur(b), font(font) { }
           DrawState()             { }
    void importer(var &d)          { }
    var  exporter()                { return null; }
    void        copy(const DrawState &r);
    
    /// io_define(DrawState, true);
    DrawState(nullptr_t n) : DrawState() { }
    DrawState(const DrawState &ref) { copy(ref);            }
    DrawState(var &d)            { importer(d);          }
    operator var()               { return exporter();    }
    operator bool()  const       { return true;          }
    bool operator!() const       { return false;         }
    DrawState &operator=(const DrawState &ref) {
        if (this != &ref)
            copy(ref);
        return *this;
    }
};

struct  Canvas;
struct ICanvasBackend {
    Canvas *host;
    var *result;
    // required implementation
    virtual void     stroke(DrawState &, Path  &) { }
    virtual void       fill(DrawState &, Path  &) { }
    virtual void       clip(DrawState &, Path  &) { }
    virtual void     stroke(DrawState &, rectd &) { }
    virtual void       fill(DrawState &, rectd &) { }
    virtual void       clip(DrawState &, rectd &) { }
    virtual void      flush(DrawState &)          { }
    virtual void *     data()                     { return null; }
    virtual vec2i      size()                     { return null; }
    virtual void       text(DrawState &, str &, rectd &, Vec2<Align> &, vec2 &, bool) { }
    virtual void    texture(DrawState &, Image *im);
    virtual void      clear(DrawState &);
    virtual void      clear(DrawState &, rgba &)   { }
    virtual TextMetrics measure(DrawState &, str &s) { assert(false); }
    
    // generically handled
    virtual void       font(DrawState &, Font  &f);
    virtual void      color(DrawState &, rgba  &);
    virtual void   gaussian(DrawState &, vec2  &, rectd);
    virtual void      scale(DrawState &, vec2  &);
    virtual void     rotate(DrawState &, double);
    virtual void  translate(DrawState &, vec2  &);
    virtual void   set_char(DrawState &, int, int, ColoredGlyph) { }
    virtual str    get_char(DrawState &, int, int) { return "";    }
    virtual str  ansi_color(rgba c, bool text)     { return null;  }
    virtual void       save(DrawState &d_state)    { }
    virtual void    restore(DrawState &d_state)    { }
    virtual void* copy_bstate(void *bs)            { return null;  }
};

struct Canvas {
public:
    enum  Type { Undefined, Terminal, Context2D };
    enum  Text { Ellipsis = 1 };
    
    Type  type =  Undefined;
protected:
    typedef std::stack<DrawState> Stack;
    ICanvasBackend   *backend = null;
    Font              default_font;
    Stack             states;

public:
    DrawState state = {};
              Canvas(nullptr_t n = nullptr);
              Canvas(vec2i sz, Type type, var *result = null);
    bool  operator==(Canvas::Type t);
    vec2i         sz();
    void      stroke(Path  &path);
    void        fill(Path  &path);
    void        clip(Path  &path);
    void      stroke(rectd &path);
    void        fill(rectd &path);
    void        clip(rectd &path);
    void       color(rgba   c);
    void       clear();
    void       clear(rgba   c);
    void    gaussian(vec2   sz, rectd cr);
    vec2i       size();
    void       scale(vec2   sc);
    void      rotate(double deg);
    void   translate(vec2   tr);
    void     texture(Image &im);
    void        font(Font &f); /// need notes for this one.
    TextMetrics measure(str s);
    void        text(str, rectd, Vec2<Align>, vec2, bool);
    void       flush();
    void   stroke_sz(double sz);
    void  font_scale(double sc);
    m44   get_matrix();
    void  set_matrix(m44 m);
    void    defaults();
    void        save();
    void     restore();
    void *      data();
    str     get_char(int x, int y);
    str   ansi_color(rgba c, bool text);
    Image   resample(vec2i size, double deg = 0.0f, rectd view = null, vec2 rc = null);
    void *copy_bstate(void *bs);
    operator     bool() { return backend != null; }
};

