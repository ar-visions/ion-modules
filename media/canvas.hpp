#pragma once
#include <dx/dx.hpp>
#include <dx/m44.hpp>b
#include <media/font.hpp>
#include <media/color.hpp>
#include <media/image.hpp>

typedef array<vec2> Points;

///
struct Canvas;
struct Align:io {
    enum Type {
        Undef,
        Start,
        Middle,
        End
    } type;
    real custom = 0;
               Align(Type t = Undef) : type(t) { }
    void        copy(const Align &ref) { type = ref.type;        }
    void importer(var &data)           { type = Type(int(data)); }
    var  exporter()                    { return int(type);       }
    bool  operator==(Type t)           { return type == t;       }
    real scale() const {
        switch (type) {
            case Undef:  return 0.0;
            case Start:  return 0.0;
            case Middle: return 0.5;
            case End:    return 1.0;
        }
        return 0.0;
    }
    io_shim(Align, type != Undef);
};

typedef Vec2<Align> AlignV2;

struct VAlign:io {
    vec2 v = { 0, 0 };
    VAlign(std::nullptr_t n = null) { }
    void init(str &d0, str &d1) {
        static str l = "left";
        static str m = "middle";
        static str t = "top";
        static str r = "right";
        static str b = "bottom";
        ///
        if (d0 == l)
            v.x = 0;
        else if (d0 == r)
            v.x = 1;
        else if (d0 == m)
            v.x = 0.5;
        else if (d1 == l)
            v.x = 0;
        else if (d1 == r)
            v.x = 1;
        else if (d1 == m)
            v.x = 0.5;
        else
            v.x = str(d0).real();
        ///
        if (d1 == t)
            v.y = 0;
        else if (d1 == b)
            v.y = 1;
        else if (d1 == m)
            v.y = 0.5;
        else if (d0 == t)
            v.y = 0;
        else if (d0 == b)
            v.y = 1;
        else if (d0 == m)
            v.y = 0.5;
        else
            v.y = str(d1).real();
    }
    VAlign(str v) { // conv to ref?
        auto sp = v.split();
        str d0 = sp.size()      ? sp[0] : "left";
        str d1 = sp.size() >= 2 ? sp[1] : "top";
        init(d0, d1);
    }
    VAlign(real x, real y) : v(x, y) { }
    VAlign(Align ax, Align ay) {
        v.x = ax.scale();
        v.y = ay.scale();
    }
    VAlign(var &d) {
        str d0 = d.size()      ? str(d[size_t(0)]) : "left";
        str d1 = d.size() >= 2 ? str(d[size_t(1)]) : "top";
        init(d0, d1);
    }
    VAlign operator*(real sc) {
        return VAlign(v.x * sc, v.y * sc);
    }
    VAlign operator+(VAlign a) {
        return VAlign(v.x + a.v.x, v.y + a.v.y);
    }
    
    operator  var  () { return v; }
    operator vec2 &() { return v; }
    operator bool  () { return true;  }
    bool operator! () { return !(operator bool()); }
};

template<> struct is_strable<VAlign> : std::true_type  {};

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
    Filter(std::nullptr_t n) : Filter(None) { }
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

struct Blending:io { /// anything we serialize is an io class
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
    
    io_shim(Blending, type >= Clear);
};

struct Cap:io {
    /// ---------------------------
    enum Type {
        None,
        Blunt,
        Round
    } value;
    /// ---------------------------
    Cap(std::nullptr_t n = null) { }
    Cap(Cap &c)             { value = c.value; }
    Cap(Type t): value(t)   { }
    Cap(var &v)             {
        str s = v;
        value = s == "round" ? Round :
                s == "blunt" ? Blunt : None;
    }
    /// ---------------------------
    bool operator==(Type t) { return value == t; }
    operator bool () { return value != None; }
    bool operator!() { return value == None; }
    Cap &operator=(const Cap &c) {
        if (&c != this)
            value = c.value;
        return *this;
    }
    operator   var() {
        str sv = str(value == Round ? "round" :
                     value == Blunt ? "blunt" : "none");
        return sv;
    }
    operator Type &() { return value; }
};


struct Join:io {
    enum Type {
        None,
        Bevel,
        Round
    } value;
    Join(std::nullptr_t n = null) { }
    Join(Join &j)            { value = j.value; }
    Join(Type t): value(t)   { }
    Join(var &v)             {
        str s = v;
        value = s == "round" ? Round :
                s == "bevel" ? Bevel : None;
    }
    bool operator==(Type t) { return value == t; }
    operator bool () { return value != None; }
    bool operator!() { return value == None; }
    Join &operator=(const Join &j) {
        if (&j != this)
            value = j.value;
        return *this;
    }
    operator   var() {
        return str(value == Round ? "round" :
                   value == Bevel ? "bevel" : "none");
    }
    operator Type &() { return value; }
};


class SkPath;
class SkPaint;
struct Path {
protected:
    SkPath *p           = null;
    SkPath *last_offset = 0;
    real    o_cache     = 0; // and that cache has this member set.
    ///
public:
    real            o   = 0; // if this is set, we contruct a new one and store in cache
    rectd        rect;
    ///
               Path();
               Path(rectd r);
               Path(const Path &ref);  //
              ~Path(); //
    Path &operator=(Path ref); //
    void       copy(Path &ref);
    Path      &move(vec2 v);
    Path      &line(vec2 v);
    Path    &bezier(vec2 cp0, vec2 cp1, vec2 p);
    Path      &quad(vec2 q0, vec2 q1);
    Path   &rect_v4(vec4 tl, vec4 tr, vec4 br, vec4 bl);
    Path &rectangle(rectd r, vec2 radius = { 0, 0 },
                    bool r_tl = true, bool r_tr = true, bool r_br = true, bool r_bl = true);
    Path       &arc(vec2 c, double radius, double rads_from, double rads, bool move_start);
    operator  rectd &();
    bool    is_rect();
    vec2         xy();
    real          w();
    real          h();
    real     aspect();
    operator   bool();
    bool  operator!();
    void set_offset(real);
    SkPath *sk_path(SkPaint *);
    bool   contains(vec2);
    rectd   &bounds();
};

struct ColoredGlyph {
    int  border;
    str  s;
    rgba bg;
    rgba fg;
    ColoredGlyph(std::nullptr_t n) : s(""), bg(null), fg(null) { }
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
    
    /// io_shim(DrawState, true);
    DrawState(std::nullptr_t n) : DrawState() { }
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
    virtual void       text(DrawState &, str &, rectd &, vec2 &, vec2 &, bool) { }
    virtual void      image(DrawState &, Image &, rectd &, vec2 &, vec2 &)     { }
    virtual void    texture(DrawState &, Image *im);
    virtual void      clear(DrawState &);
    virtual void      clear(DrawState &, rgba &)   { }
    virtual TextMetrics measure(DrawState &, str &s) { assert(false); }
    
    // generically handled
    virtual void        cap(DrawState &, Cap::Type)  { } /// no-op for terminal
    virtual void       join(DrawState &, Join::Type) { } /// no-op for terminal
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
              Canvas(std::nullptr_t n = nullptr);
              Canvas(vec2i sz, Type type, var *result = null);
    bool  operator==(Canvas::Type t);
    vec2i         sz();
    void      stroke(Path  &path);
    void        fill(Path  &path);
    void         cap(Cap::Type cap);
    void        join(Join::Type join);
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
    void        text(str, rectd, vec2, vec2, bool);
    void       image(Image &, rectd, vec2, vec2);
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

