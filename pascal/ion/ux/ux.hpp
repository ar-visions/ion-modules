#pragma once

#include <core/core.hpp>
#include <math/math.hpp>
#include <async/async.hpp>
#include <image/image.hpp>

struct Device;
struct Texture;
struct GLFWwindow;
struct Texture;
struct BufferMemory;
struct GPU;

using Path    = path;
using Image   = image;
using strings = array<str>;
using Assets  = map<Texture *>;


struct text_metrics:mx {
    struct data {
        real           w, h;
        real         ascent,
                    descent;
        real    line_height,
                 cap_height;
    } &m;
    ctr(text_metrics, mx, data, m);
};

using tm_t = text_metrics;

/// would be nice to handle basically everything model-wise in graphics
namespace graphics {
    enums(cap, none,
       "none, blunt, round",
        none, blunt, round);
    
    enums(join, miter,
       "miter, round, bevel",
        miter, round, bevel);

    struct arc:mx {
        struct data {
            vec2 cen;
            real radius;
            vec2 degs; /// x:from [->y:distance]
            vec2 origin;
        } &m;
        ctr(arc, mx, data, m);
    };

    struct line:mx {
        struct data {
            vec2 to;
            vec2 origin;
        } &m;
        ctr(line, mx, data, m);
    };

    struct move:mx {
        struct data {
            vec2 to;
        } &m;
        ctr(move, mx, data, m);
    };

    struct bezier:mx {
        struct data {
            vec2 cp0;
            vec2 cp1;
            vec2 b;
            vec2 origin;
        } &m;
        ctr(bezier, mx, data, m);
    };

    struct rect:mx {
        using T  = real; /// i cannot believe its not a template type
        using r4 = r4<T>;
        using v2 = v2<T>;
        
        r4 &data;

        ctr(rect, mx, r4, data); 

        inline rect(T x, T y, T w, T h) : rect(r4 { x, y, w, h }) { }
        inline rect(v2 p0, v2 p1)       : rect(p0.x, p0.y, p1.x, p1.y) { }

        inline rect  offset(T a)               const { return data.offset(a); }
        inline size       shape()              const { return { num(data.h), num(data.w) }; }
        inline v2          size()              const { return data.size();       }
        inline v2            xy()              const { return data.xy();         }
        inline v2        center()              const { return data.center();     }
        inline bool    contains(vec2 p)        const { return data.contains(p);  }
        inline bool  operator==(const rect &r) const { return   data == r.data;  }
        inline bool  operator!=(const rect &r) const { return !(data == r.data); }
        inline rect operator +(rect         r) const { return data + r.data;     }
        inline rect operator -(rect         r) const { return data - r.data;     }
        inline rect operator +(v2           v) const { return { data.x + v.x, data.y + v.y, data.w, data.h }; }
        inline rect operator -(v2           v) const { return { data.x - v.x, data.y - v.y, data.w, data.h }; }
        inline rect operator *(T            r) const { return data * r;          }
        inline rect operator /(T            r) const { return data / r;          }
        inline      operator  bool          () const { return bool(data);        }

        array<v2> edge_points() const {
            return { {data.x,data.y}, {data.x+data.w,data.y}, {data.x+data.w,data.y+data.h}, {data.x,data.y+data.h} };
        }
        
        array<v2> clip(array<v2> &poly) const {
            const rect  &clip = *this;
            array<v2>        p = poly;
            array<v2>   points = clip.edge_points();
            ///
            for (int i = 0; i < points.len(); i++) {
                v2   &e0   = points[i];
                v2   &e1   = points[(i + 1) % points.len()];
                e2<T> edge = { e0, e1 };
                array<v2> cl;
                ///
                for (int ii = 0; ii < p.len(); ii++) {
                    const v2 &pi = p[(ii + 0)];
                    const v2 &pk = p[(ii + 1) % p.len()];
                    const bool    top = i == 0;
                    const bool    bot = i == 2;
                    const bool    lft = i == 3;
                    ///
                    if (top || bot) {
                        const bool cci = top ? (edge.a.y <= pi.y) : (edge.a.y > pi.y);
                        const bool cck = top ? (edge.a.y <= pk.y) : (edge.a.y > pk.y);
                        if (cci) {
                            cl += cck ? pk : v2 { edge.x(pi, pk), edge.a.y };
                        } else if (cck) {
                            cl += v2 { edge.x(pi, pk), edge.a.y };
                            cl += pk;
                        }
                    } else {
                        const bool cci = lft ? (edge.a.x <= pi.x) : (edge.a.x > pi.x);
                        const bool cck = lft ? (edge.a.x <= pk.x) : (edge.a.x > pk.x);
                        if (cci) {
                            cl += cck ? pk : v2 { edge.a.x, edge.y(pi, pk) };
                        } else if (cck) {
                            cl += v2 { edge.a.x, edge.y(pi, pk) };
                            cl += pk;
                        }
                    }
                }
                p = cl;
            }
            return p;
        }
    };

    /// a more distortable primitive. its also distorted in code
    /// from these you can form the args needed for bezier and line-paths
    struct rounded:rect {
        struct data {

            v2r  p_tl, v_tl, d_tl;
            real l_tl, l_tr, l_br, l_bl;
            v2r  p_tr, v_tr, d_tr;
            v2r  p_br, v_br, d_br;
            v2r  p_bl, v_bl, d_bl;
            v2r  r_tl, r_tr, r_br, r_bl;

            /// pos +/- [ dir * scale ]
            v2r tl_x, tl_y, tr_x, tr_y, br_x, br_y, bl_x, bl_y;
            v2r c0, c1, p1, c0b, c1b, c0c, c1c, c0d, c1d;

            data() { } /// this is implicit zero fill
            data(v4r tl, v4r tr, v4r br, v4r bl) {
                /// top-left
                p_tl  = tl.xy();
                v_tl  = tr.xy() - p_tl;
                l_tl  = v_tl.len();
                d_tl  = v_tl / l_tl;

                /// top-right
                p_tr  = tr.xy();
                v_tr  = br.xy() - p_tr;
                l_tr  = v_tr.len();
                d_tr  = v_tr / l_tr;

                // bottom-right
                p_br  = br.xy();
                v_br  = bl.xy() - p_br;
                l_br  = v_br.len();
                d_br  = v_br / l_br;

                /// bottom-left
                p_bl  = bl.xy();
                v_bl  = tl.xy() - p_bl;
                l_bl  = v_bl.len();
                d_bl  = v_bl / l_bl;

                /// set-radius
                r_tl  = { math::min(tl.w, l_tl / 2), math::min(tl.z, l_bl / 2) };
                r_tr  = { math::min(tr.w, l_tr / 2), math::min(tr.z, l_br / 2) };
                r_br  = { math::min(br.w, l_br / 2), math::min(br.z, l_tr / 2) };
                r_bl  = { math::min(bl.w, l_bl / 2), math::min(bl.z, l_tl / 2) };
                
                /// pos +/- [ dir * radius ]
                tl_x = p_tl + d_tl * r_tl;
                tl_y = p_tl - d_bl * r_tl;
                tr_x = p_tr - d_tl * r_tr;
                tr_y = p_tr + d_tr * r_tr;
                br_x = p_br + d_br * r_br;
                br_y = p_br + d_bl * r_br;
                bl_x = p_bl - d_br * r_bl;
                bl_y = p_bl - d_tr * r_bl;

                c0   = (p_tr + tr_x) / 2;
                c1   = (p_tr + tr_y) / 2;
                p1   =  tr_y;
                c0b  = (p_br + br_y) / 2;
                c1b  = (p_br + br_x) / 2;
                c0c  = (p_bl + bl_x) / 2;
                c1c  = (p_bl + bl_y) / 2;
                c0d  = (p_tl + bl_x) / 2;
                c1d  = (p_bl + bl_y) / 2;
            }

            data(r4r &r, real rx, real ry)
                : data(vec4 {r.x, r.y, rx, ry},             vec4 {r.x + r.w, r.y, rx, ry},
                       vec4 {r.x + r.w, r.y + r.h, rx, ry}, vec4 {r.x, r.y + r.h, rx, ry}) { }
            
        } &m;
        
        ctr(rounded, rect, data, m);
        
        /// needs routines for all prims
        inline bool contains(vec2 v) { return (v >= m.p_tl && v < m.p_br); }
        ///
        real  w() { return m.p_br.x - m.p_tl.x; }
        real  h() { return m.p_br.y - m.p_tl.y; }
        vec2 xy() { return m.p_tl; }

        operator bool() { return m.l_tl <= 0; }

        /// set og rect (r4r) and compute the bezier
        rounded(r4r &r, real rx, real ry) : rounded(data { r, rx, ry }) {
            rect::data = r;
        }
    };

    struct movement:vec2 {
        ictr(movement, vec2);
    };

    struct shape:mx {
        struct data {
            rect       bounds;
            doubly<mx> ops;
        } &m;
        ///
        ctr(shape, mx, data, m);

        bool contains(vec2 p) {
            if (!m.bounds)
                bounds();
            return m.bounds.contains(p);
        }
        ///
        rect bounds() {
            real min_x = 0, max_x = 0;
            real min_y = 0, max_y = 0;
            int  index = 0;
            ///
            if (!m.bounds && m.ops) {
                /// get aabb from vec2
                for (mx &v:m.ops) {
                    v2r *vd = v.get<v2r>(0); 
                    if (!vd) continue;
                    if (!index || vd->x < min_x) min_x = vd->x;
                    if (!index || vd->y < min_y) min_y = vd->y;
                    if (!index || vd->x > max_x) max_x = vd->x;
                    if (!index || vd->y > max_y) max_y = vd->y;
                    index++;
                }
            }
            /// null bounds is possible and thats by design on shape,
            /// which, can simply have no shape at all
            return m.bounds;
        }

        shape(r4r r4, real rx = nan<real>(), real ry = nan<real>()) : shape() {
            bool use_rect = isnan(rx) || rx == 0 || ry == 0;
            m.bounds = use_rect ? graphics::rect(graphics::rect   (r4)) : 
                                  graphics::rect(graphics::rounded(r4, rx, isnan(ry) ? rx : ry));
        }



        bool is_rect () { return m.bounds.type() == typeof(r4r)           && !m.ops; }
        bool is_round() { return m.bounds.type() == typeof(rounded::data) && !m.ops; }    
        void move    (vec2 v) { m.ops += movement(v); } /// following ops are easier this way than having a last position which has its exceptions for arc/line primitives
        void line    (vec2 l) { m.ops += l; }
        void bezier  (graphics::bezier b) { m.ops += b; }
      //void quad    (graphics::quad   q) { m.ops += q; }
        void arc     (graphics::arc    a) { m.ops += a; }
        graphics::rect rect() {
            return bounds();
        };
        
        operator       bool() { bounds(); return bool(m.bounds); }
        bool      operator!() { return !operator bool(); }
        shape::data *handle() { return &m; }
    };

    struct border_data {
        real size, tl, tr, bl, br;
        rgba color;
        inline bool operator==(const border_data &b) const { return b.tl == tl && b.tr == tr && b.bl == bl && b.br == br; }
        inline bool operator!=(const border_data &b) const { return !operator==(b); }

        /// members are always null from the memory::allocation
        border_data() { }
        ///
        border_data(str raw) : border_data() {
            str        trimmed = raw.trim();
            size_t     tlen    = trimmed.len();
            array<str> values  = raw.split();
            size_t     ncomps  = values.len();
            ///
            if (tlen > 0) {
                size = values[0].real_value<real>();
                if (ncomps == 5) {
                    tl = values[1].real_value<real>();
                    tr = values[2].real_value<real>();
                    br = values[3].real_value<real>();
                    bl = values[4].real_value<real>();
                } else if (tlen > 0 && ncomps == 2)
                    tl = tr = bl = br = values[1].real_value<real>();
                else
                    console.fault("border requires 1 (size) or 2 (size, roundness) or 5 (size, tl tr br bl) values");
            }
        }
    };
    /// graphics::border is an indication that this has information on color
    using border = sp<border_data>;
};

enums(nil, none, "none", none);

enums(xalign, undefined,
    "undefined, left, middle, right",
     undefined, left, middle, right);
    
enums(yalign, undefined,
    "undefined, top, middle, bottom, line",
     undefined, top, middle, bottom, line);

enums(blending, undefined,
    "undefined, clear, src, dst, src-over, dst-over, src-in, dst-in, src-out, dst-out, src-atop, dst-atop, xor, plus, modulate, screen, overlay, color-dodge, color-burn, hard-light, soft-light, difference, exclusion, multiply, hue, saturation, color",
     undefined, clear, src, dst, src_over, dst_over, src_in, dst_in, src_out, dst_out, src_atop, dst_atop, Xor, plus, modulate, screen, overlay, color_dodge, color_burn, hard_light, soft_light, difference, exclusion, multiply, hue, saturation, color);

enums(filter, none,
    "none, blur, brightness, contrast, drop-shadow, grayscale, hue-rotate, invert, opacity, saturate, sepia",
     none, blur, brightness, contrast, drop_shadow, grayscale, hue_rotate, invert, opacity, saturate, sepia);

enums(duration, undefined,
    "undefined, ns, ms, s",
     undefined, ns, ms, s);

/// needs more distance units.  pc = parsec
enums(distance, undefined,
    "undefined, px, m, cm, in, ft, pc, %",
     undefined, px, m, cm, in, ft, parsec, percent);

/// x20cm, t10% things like this.
template <typename P, typename S>
struct scalar:mx {
    using scalar_t = scalar<P, S>;
    struct data {
        P            prefix;
        real         scale;
        S            suffix;
        bool         is_percent;
    } &m;

    ctr(scalar, mx, data, m);

    real operator()(real origin, real size) {
        return origin + (m.is_percent ? (m.scale / 100.0 * size) : m.scale);
    }

    using p_type = typename P::etype;
    using s_type = typename S::etype;

    inline bool operator>=(p_type p) const { return p >= m.prefix; }
    inline bool operator> (p_type p) const { return p >  m.prefix; }
    inline bool operator<=(p_type p) const { return p <= m.prefix; }
    inline bool operator< (p_type p) const { return p <  m.prefix; }
    inline bool operator>=(s_type s) const { return s >= m.suffix; }
    inline bool operator> (s_type s) const { return s >  m.suffix; }
    inline bool operator<=(s_type s) const { return s <= m.suffix; }
    inline bool operator< (s_type s) const { return s <  m.suffix; }
    inline bool operator==(p_type p) const { return p == m.prefix; }
    inline bool operator==(s_type s) const { return s == m.suffix; }
    inline bool operator!=(p_type p) const { return p != m.prefix; }
    inline bool operator!=(s_type s) const { return s != m.suffix; }

    inline operator    P() const { return p_type(m.prefix); }
    inline operator    S() const { return s_type(m.suffix); }
    inline operator real() const { return m.scale; }

    scalar(p_type prefix, real scale, s_type suffix) : scalar(data { prefix, scale, suffix, false }) { }

    scalar(str s) : scalar() {
        str    tr  = s.trim();
        size_t len = tr.len();
        bool in_symbol = isalpha(tr[0]) || tr[0] == '_' || tr[0] == '%'; /// str[0] is always guaranteed to be there
        array<str> sp   = tr.split([&](char &c) -> bool {
            bool split  = false;
            bool symbol = isalpha(c) || (c == '_' || c == '%'); /// todo: will require hashing the enums by - and _, but for now not exposed as issue
            ///
            if (c == 0 || in_symbol != symbol) {
                in_symbol = symbol;
                split     = true;
            }
            return split;
        });

        try {
            if constexpr (identical<P, nil>()) {
                m.scale  =   sp[0].real_value<real>();
                if (sp[1]) {
                    m.suffix = S(sp[1]);
                    if constexpr (identical<S, distance>())
                        m.is_percent = m.suffix == distance::percent;
                }
            } else if (identical<S, nil>()) {
                m.prefix = P(sp[0]);
                m.scale  =   sp[1].real_value<real>();
            } else {
                m.prefix = P(sp[0]);
                m.scale  =   sp[1].real_value<real>();
                if (sp[2]) {
                    m.suffix = S(sp[2]);
                    if constexpr (identical<S, distance>())
                        m.is_percent = m.suffix == distance::percent;
                }
                m.prefix = P(sp[0]);
                m.scale  =   sp[1].real_value<real>();
            }
        } catch (P &e) {
            console.fault("type exception in scalar construction: prefix lookup failure: {0}", { str(e.mx::type()->name) });
        } catch (S &e) {
            console.fault("type exception in scalar construction: suffix lookup failure: {0}", { str(e.type()->name) });
        }
    }
};

///
struct alignment:mx {
    struct data {
        scalar<xalign, distance> x; /// these are different types
        scalar<yalign, distance> y; /// ...
    } &coords;

    ///
    ctr(alignment, mx, data, coords);

    ///
    inline alignment(str  x, str  y) : alignment { data { x, y } } { }
    inline alignment(real x, real y)
         : alignment { data { { xalign::left, x, distance::px },
                              { yalign::top,  y, distance::px } } } { }

    inline operator vec2() { return vec2 { real(coords.x), real(coords.y) }; }

    /// compute x and y coordinates given alignment data, and parameters for rect and offset
    vec2 plot(r4<real> &win) {
        v2<real> rs;
        /// todo: direct enum operator does not seem to works
        /// set x coordinate
        switch (xalign::etype(xalign(coords.x))) {
            case xalign::undefined: rs.x = nan<real>();                        break;
            case xalign::left:      rs.x = coords.x(win.x,             win.w); break;
            case xalign::middle:    rs.x = coords.x(win.x + win.w / 2, win.w); break;
            case xalign::right:     rs.x = coords.x(win.x + win.w,     win.w); break;
        }
        /// set y coordinate
        switch (yalign::etype(yalign(coords.y))) {
            case yalign::undefined: rs.y = nan<real>();                        break;
            case yalign::top:       rs.y = coords.y(win.y,             win.h); break;
            case yalign::middle:    rs.y = coords.y(win.y + win.h / 2, win.h); break;
            case yalign::bottom:    rs.y = coords.y(win.y + win.h,     win.h); break;
            case yalign::line:      rs.y = coords.y(win.y + win.h / 2, win.h); break;
        }
        /// return vector of this 'corner' (top-left or bottom-right)
        return vec2(rs);
    }
};

/// good primitive for ui, implemented in basic canvas ops.
/// regions can be constructed from rects if area is static or composed in another way
struct region:mx {
    struct data {
        alignment tl;
        alignment br;
    } &m;

    ///
    ctr(region, mx, data, m);
    
    ///
    region(str s) : region() {
        array<str> a = s.trim().split();
        m.tl = alignment { a[0], a[1] };
        m.br = alignment { a[2], a[3] };
    }

    /// when given a shape, this is in-effect a window which has static bounds
    /// this is used to convert shape abstract to a region
    region(graphics::shape sh) : region() {
        r4r &b = sh.bounds();
        m.tl   = alignment { b.x,  b.y };
        m.br   = alignment { b.x + b.w,
                             b.y + b.h };
    }
    
    ///
    r4<real> rect(r4<real> &win) {
        return { m.tl.plot(win), m.br.plot(win) };
    }
};

enums(keyboard, none,
    "none, caps_lock, shift, ctrl, alt, meta",
     none, caps_lock, shift, ctrl, alt, meta);

enums(mouse, none,
    "none, left, right, middle, inactive",
     none, left, right, middle, inactive);

namespace user {
    using chr = num;

    struct key:mx {
        struct data {
            num               unicode;
            num               scan_code;
            states<keyboard>  modifiers;
            bool              repeat;
            bool              up;
        } &m;
        ctr(key, mx, data, m);
    };
};

struct event:mx {
    ///
    struct edata {
        user::chr     chr;
        user::key     key;
        vec2          cursor;
        states<mouse>    buttons;
        states<keyboard> modifiers;
        size          resize;
        mouse::etype  button_id;
        bool          prevent_default;
        bool          stop_propagation;
    } &evd;

    ///
    ctr(event, mx, edata, evd);
    
    ///
    inline void prevent_default()   {         evd.prevent_default = true; }
    inline bool is_default()        { return !evd.prevent_default; }
    inline bool should_propagate()  { return !evd.stop_propagation; }
    inline bool stop_propagation()  { return  evd.stop_propagation = true; }
    inline vec2 cursor_pos()        { return  evd.cursor; }
    inline bool mouse_down(mouse m) { return  evd.buttons[m]; }
    inline bool mouse_up  (mouse m) { return !evd.buttons[m]; }
    inline num  unicode   ()        { return  evd.key->unicode; }
    inline num  scan_code ()        { return  evd.key->scan_code; }
    inline bool key_down  (num u)   { return  evd.key->unicode   == u && !evd.key->up; }
    inline bool key_up    (num u)   { return  evd.key->unicode   == u &&  evd.key->up; }
    inline bool scan_down (num s)   { return  evd.key->scan_code == s && !evd.key->up; }
    inline bool scan_up   (num s)   { return  evd.key->scan_code == s &&  evd.key->up; }
};


struct node;
struct style;

/// enumerables for the state bits
enums(interaction, undefined,
    "undefined, captured, focused, hover, active, cursor",
     undefined, captured, focused, hover, active, cursor);

struct   map_results:mx {   map_results():mx() { } };
struct array_results:mx { array_results():mx() { } };

struct Element:mx {
    struct data {
        type_t           type;     /// type given
        memory*          id;       /// identifier 
        ax               args;     /// ax = better than args.  we use args as a var all the time; arguments is terrible
        array<Element>  *children; /// lol.
        node*            instance; /// node instance is 1:1
        map<node*>       mounts;   /// store instances of nodes in element data, so the cache management can go here where element turns to node
        node*            parent;
        style*           root_style; /// applied at root for multiple style trees across multiple apps
        states<interaction> istates;
    } &e;

    /// default case when there is no render()
    array<Element> &children() { return *e.children; }

    ctr(Element, mx, data, e);

    static memory *args_id(type_t type, initial<arg> args) {
        static memory *m_id = memory::symbol("id"); /// im a token!
        for (auto &a:args)
            if (a.key.mem == m_id)
                return a.value.mem;
        
        return memory::symbol(symbol(type->name));
    }

    //inline Element(ax &a):mx(a.mem->grab()), e(defaults<data>()) { }
    Element(type_t type, initial<arg> args) : Element(
        data { type, args_id(type, args), args } // ax = array of args; i dont want the different ones but they do syn
    ) { }

    /// inline expression of the data inside, im sure this would be common case (me 6wks later: WHERE????)
    Element(type_t type, size_t sz) : Element(
        data { type, null, null, new array<Element>(sz) }
    ) { }

    template <typename T>
    static Element each(array<T> a, lambda<Element(T &v)> fn) {
        Element res(typeof(map_results), a.len());
        for (auto &v:a) {
            Element ve = fn(v);
            if (ve) *res.e.children += ve;
        }
        return res;
    }
    
    template <typename K, typename V>
    static Element each(map<V> m, lambda<Element(K &k, V &v)> fn) {
        Element res(typeof(array_results), m.size);
        if (res.e.children)
        for (auto &[v,k]:m) {
            Element r = fn(k, v);
            if (r) *res.e.children += r;
        }
        return res;
    }

    mx node() {
        assert(false);
        return mx();
        ///return type->ctr();
    }
};

using elements  = array<Element>;
using fn_render = lambda<Element()>;
using fn_stub   = lambda<void()>;
using callback  = lambda<void(event)>;

struct dispatch;

struct listener:mx {
    struct data {
        dispatch *src;
        callback  cb;
        fn_stub   detatch;
        bool      detached;
        ///
        ~data() { printf("listener, ...destroyed\n"); }
    } &m;
    
    ///
    ctr(listener, mx, data, m);

    void cancel() {
        m.detatch();
        m.detached = true;
    }

    ///
    ~listener() {
        if (!m.detached && mem->refs == 1) /// only reference is the binding reference, detect and make that ref:0, ~data() should be fired when its successfully removed from the list
            cancel();
    }
};

/// keep it simple for sending events, with a dispatch.  listen to them with listen()
struct dispatch:mx {
    struct data {
        doubly<listener> listeners;
    } &m;
    ///
    ctr(dispatch, mx, data, m);
    ///
    void operator()(event e);
    ///
    listener &listen(callback cb);
};

/// simple wavefront obj
template <typename V>
struct Obj:mx {
    /// prefer this to typedef
    using strings = array<str>;

    struct group:mx {
        struct data {
            str        name;
            array<u32> ibo;
        } &m;
        ctr(group, mx, data, m);
    };

    struct members {
        array<V>   vbo;
        map<group> groups;
    } &m;

    ctr(Obj, mx, members, m);

    Obj(path p, lambda<V(group&, vec3&, vec2&, vec3&)> fn) : Obj() {
        str g;
        str contents  = str::read_file(p.exists() ? p : fmt {"models/{0}.obj", { p }});
        assert(contents.len() > 0);
        
        auto lines    = contents.split("\n");
        size_t line_c = lines.len();
        auto wlines   = array<strings>();
        auto v        = array<vec3>(line_c); // need these
        auto vn       = array<vec3>(line_c);
        auto vt       = array<vec2>(line_c);
        auto indices  = std::unordered_map<str, u32>(); ///
        size_t verts  = 0;

        ///
        for (str &l:lines)
            wlines += l.split(" ");
        
        /// assert triangles as we count them
        map<int> gcount = {};
        for (strings &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                m.groups[g].name  = g;
                m.gcount[g]       = 0;
            } else if (w[0] == "f") {
                if (!g.len() || w.len() != 4)
                    console.fault("import requires triangles"); /// f pos/uv/norm pos/uv/norm pos/uv/norm
                m.gcount[g]++;
            }
        }

        /// add vertex data
        for (auto &w: wlines) {
            if (w[0] == "g" || w[0] == "o") {
                g = w[1];
                if (!m.groups[g].ibo)
                     m.groups[g].ibo = array<u32>(m.gcount[g] * 3);
            }
            else if (w[0] == "v")  v  += vec3 { w[1].real_value<real>(), w[2].real_value<real>(), w[3].real_value<real>() };
            else if (w[0] == "vt") vt += vec2 { w[1].real_value<real>(), w[2].real_value<real>() };
            else if (w[0] == "vn") vn += vec3 { w[1].real_value<real>(), w[2].real_value<real>(), w[3].real_value<real>() };
            else if (w[0] == "f") {
                assert(g.len());
                for (size_t i = 1; i < 4; i++) {
                    auto key = w[i];
                    if (indices.count(key) == 0) {
                        indices[key] = u32(verts++);
                        auto      sp =  w[i].split("/");
                        int      iv  = sp[0].integer_value();
                        int      ivt = sp[1].integer_value();
                        int      ivn = sp[2].integer_value();
                        m.vbo       += fn(m.groups[g], iv ?  v[ iv-1] : null,
                                                      ivt ? vt[ivt-1] : null,
                                                      ivn ? vn[ivn-1] : null);
                    }
                    m.groups[g].ibo += indices[key];
                }
            }
        }
    }
};

enums(mode, regular,
     "regular, headless",
      regular, headless);

struct window_data;
struct window:mx {
    window_data *w;
    ptr_decl(window, mx, window_data, w);

    Device   *device();
    Texture  &texture();
    Texture  &texture(::size sz);
    
    void loop(lambda<void()> fn);

    window(size sz, mode::etype m, memory *control);

    inline static window handle(GLFWwindow *h);

    bool key(int key);

    vec2 cursor();
    str  clipboard();
    void set_clipboard(str text);
    void set_title(str s);
    void show();
    void hide();
    void start();
  ::size size();
    void repaint();
    operator bool();
};

struct BufferMemory;
struct Buffer:mx {
    enums(Usage, Undefined,
       "Undefined, Src, Dst, Uniform, Storage, Index, Vertex",
        Undefined, Src, Dst, Uniform, Storage, Index, Vertex);
    ///
    BufferMemory *bmem;
    //operators(Buffer, bmem);
    void destroy();

    ///
    Buffer(Device *, size_t type_size, void *data, size_t count);

    template <typename T>
    Buffer(Device *d, array<T> &v, Usage usage): Buffer(create_buffer(d, v.len(), (void*)v.data(), typeof(T), usage)) { }

    ptr_decl(Buffer, mx, BufferMemory, bmem);
    
    size_t count();
    size_t type_size();
};

BufferMemory *create_buffer(Device *d, size_t sz, void *bytes, type_t type, Buffer::Usage usage);

struct IndexMemory;
///
struct IndexData:mx {
    IndexMemory *imem;
    ptr_decl(IndexData, mx, IndexMemory, imem);
    size_t len();
    IndexData(Device *device, Buffer buffer);
    operator bool ();
    bool operator!();
};

///
template <typename I>
struct IndexBuffer:IndexData {
    //VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
    //    return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
    //             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    //}
    IndexBuffer(std::nullptr_t n = null) : IndexData(n) { }
    IndexBuffer(Device *device, array<I> &i) : IndexData(device, Buffer {
            device, i, Buffer::Usage::Index }) { }
};

///
struct VertexData:mx {
    struct VertexInternal *m;
    ptr_decl(VertexData, mx, VertexInternal, m);

    VertexData(Device *device, Buffer buffer, lambda<void(void*)> fn_attribs);
    ///
    size_t       size();
       operator bool ();
       bool operator!();
};

enums(Asset, Undefined,
    "Undefined, Color, Normal, Specular, Displace",
     Undefined, Color, Normal, Specular, Displace);

enums(VA, Position,
    "Position, Normal, UV, Color, Tangent, BiTangent",
     Position, Normal, UV, Color, Tangent, BiTangent);

using VAttribs = states<VA>;

struct VkWriteDescriptorSet;
//struct VkDescriptorSet;

struct TextureMemory;
struct StageData;
///
struct Texture:mx {
    struct Stage {
        enum Type {
            Undefined,
            Transfer,
            Shader,
            Color,
            Depth
        };
        ///
        Type value;

        const StageData *data();
        ///
        inline bool operator==(Stage &b)      { return value == b.value; }
        inline bool operator!=(Stage &b)      { return value != b.value; }
        inline bool operator==(Stage::Type b) { return value == b; }
        inline bool operator!=(Stage::Type b) { return value != b; }
        ///
        Stage(      Type    type = Undefined);
        Stage(const Stage & ref) : value(ref.value) { };
        Stage(      Stage & ref) : value(ref.value) { };
        ///
        bool operator>(Stage &b) { return value > b.value; }
        bool operator<(Stage &b) { return value < b.value; }
             operator     Type() { return value; }
    };
    TextureMemory *data;
    ptr_decl(Texture, mx, TextureMemory, data);

    void set_stage(Stage s) ;
    void push_stage(Stage s);
    void pop_stage();
    void transfer_pixels(rgba::data *pixels);
    
public:
    /// pass-through operators
    operator  bool                  () { return  data; }
    bool operator!                  () { return !data; }
    bool operator==      (Texture &tx) { return &data == &tx.data; }
};

struct PipelineData;
struct Texture;

struct gpu_memory;
template <> struct is_opaque<gpu_memory> : true_type { };

struct GPU:mx {
    enum Capability {
        Present,
        Graphics,
        Complete
    };

    gpu_memory *gmem; // support this.
    ptr_decl(GPU, mx, gpu_memory, gmem);
    //operators(GPU, gmem);

    uint32_t  index(Capability);
    void      destroy();
    operator  bool();
    bool      operator()(Capability);
};

struct Device;

using  UniformFn = lambda<void(void *)> ;

struct UniformMemory;

struct UniformData:mx {
    UniformMemory *umem;
    ptr_decl(UniformData, mx, UniformMemory, umem);

    UniformData(Device *device, size_t struct_sz, UniformFn fn);

    inline operator  bool();
    inline bool operator!();
};

template <typename U>
struct UniformBuffer:UniformData { /// will be best to call it 'Uniform', 'Verts', 'Polys'; make sensible.
    UniformBuffer(std::nullptr_t n = null) { }
    UniformBuffer(Device *device, UniformFn fn) : UniformData(device, sizeof(U), fn) { }
};


struct Light {
    v4f pos_rads;
    v4f color;
};

///
struct Material {
    v4f ambient;
    v4f diffuse;
    v4f specular;
    v4f attr;
};

///
struct MVP {
    m44f::data model;
    m44f::data view;
    m44f::data proj;
};

struct vkState;
using VkStateFn = lambda<void(vkState *)>;

struct PipelineMemory;

/// pipeline data is the base on Pipeline interface, consisting of an internal implementation in PipelineMemory
struct PipelineData:mx {
    PipelineMemory *m;
    ptr_decl(PipelineData, mx, PipelineMemory, m);
    ///
    operator bool  ();
    bool operator! ();
    void   enable  (bool en);
    bool operator==(PipelineData &b);
    void   update  (size_t frame_id);
    ///
    PipelineData(Device *device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 Assets &assets, size_t vsize, rgba clr, str shader, VkStateFn vk_state = null);
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device *device, UniformData &ubo,  VertexData &vbo,
             IndexData &ibo,   Assets &assets,  rgba clr, str name):
        PipelineData(device, ubo, vbo, ibo, assets, sizeof(V), clr, name) { }
};

/// calls for Daniel
struct Pipes:mx {
    struct data {
        Device           *device  = null;
        VertexData        vbo     = null;
        uint32_t          binding = 0;
      //array<Attrib>     attr    = {};
        map<PipelineData> part;
    } &d;
    
    ctr(Pipes, mx, data, d);

    Pipes(Device *device);

    /// could be a global mx resource, so domain, name, attribute, extension
    static str form_path(str model, str skin, str ext) {
        str sk = skin ? str(skin + ".") : str("");
        return fmt {"textures/{0}{1}.{2}", {model, sk, ext}};
    }

    /// skin will just override where overriden in the file-system
    static Assets cache_assets(data &d, str model, str skin, states<Asset> &atypes);
    
    operator bool() { return d.part.count() > 0; }
    inline map<PipelineData> &map() { return d.part; }
};

/// generic vertex model; uses spec map, normal map, 
struct Vertex {
    ///
    v3f pos;  // position
    v3f norm; // normal position
    v2f uv;   // texture uv coordinate
    v4f clr;  // color
    v3f ta;   // tangent
    v3f bt;   // bi-tangent, we need an arb in here, or two. or ten.
    
    /// VkAttribs (array of VkAttribute) [data] from VAttribs (state flags) [model]
    static void attribs(VAttribs attr, void *vk_attr_res);
    
    Vertex() { }
    ///
    Vertex(vec3f pos, vec3f norm, vec2f uv, vec4f clr, vec3f ta = {}, vec3f bt = {}):
           pos  (pos.data), norm (norm.data), uv   (uv.data),
           clr  (clr.data), ta   (ta.data),   bt   (bt.data) { }
    
    ///
    Vertex(vec3f &pos, vec3f &norm, vec2f &uv, vec4f &clr, vec3f &ta, vec3f &bt):
           pos  (pos.data), norm (norm.data), uv   (uv.data),
           clr  (clr.data), ta   (ta.data),   bt   (bt.data) { }
    
    /// hip to consider this data
    static array<Vertex> square(vec4f v_clr = {1.0, 1.0, 1.0, 1.0}) {
        return array<Vertex> {
            Vertex {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, v_clr},
            Vertex {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, v_clr},
            Vertex {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, v_clr},
            Vertex {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, v_clr}
        };
    }
};

template <typename V>
struct VertexBuffer:VertexData {
    VertexBuffer(Device *device, array<V> &v, VAttribs &attr) :
        VertexData(device, Buffer {
            device, v, Buffer::Usage::Src },
            [attr=attr](void *vk_attr_res) { return V::attribs(attr, vk_attr_res); }) { }
};

///
struct Shaders {
    map<str> map;
    /// default construction
    Shaders(std::nullptr_t n = null) {
        map["*"] = "main";
    }
    
    bool operator==(Shaders &ref) {
        return map == ref.map;
    }
    
    operator bool()  { return  map.count(); }
    bool operator!() { return !map.count(); }
    
    /// group=shader
    Shaders(str v) { /// str is the only interface in it.  everything else is just too messy for how simple the map is
        strings sp = v.split(",");
        for (str &v: sp) {
            auto a = v.split("=");
            assert(a.len() == 2);
            str key   = a[0];
            str value = a[1];
            map[key] = value;
        }
    }
    str operator()(str &group) {
        return map.count(group) ? map[group] : map["*"];
    }
    str &operator[](str n) {
        return map[n];
    }
    size_t count(str n) {
        return map.count(n);
    }
};

struct Device;
struct Texture;

template <> struct is_opaque<struct BufferMemory>   : true_type { };
template <> struct is_opaque<struct UniformMemory>  : true_type { };
template <> struct is_opaque<struct DeviceMemory>   : true_type { };
template <> struct is_opaque<struct PipelineMemory> : true_type { };

struct Composer;

extern Assets cache_assets(str model, str skin, states<Asset> &atypes);

/// Model is the integration of device, ubo, attrib, with interfaces around OBJ format or Lambda
template <typename V>
struct Model:Pipes {
    struct Polys {
        ::map<array<int32_t>> groups;
        array<V> verts;
    };
    
    ///
    struct Shape {
        using ModelFn = lambda<Polys(void)>;
        ModelFn fn;
    };
    
    ///
    Model(Device *device) : Pipes(device) { }
    
    Model(str name, str skin, Device &device, UniformData &ubo, VAttribs &attr,
          states<Asset> &atypes, Shaders &shaders) : Model(device) {
        using Obj = Obj<V>;
        /// cache control for images to texture here; they might need a new reference upon pipeline rebuild
        auto assets = Pipes::cache_assets(d, name, skin, atypes);
        auto  mpath = form_path(name, skin, ".obj");
        auto    obj = Obj(mpath, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        auto &d = *this->data;
        // VertexBuffer(Device &device, array<V> &v, VAttribs &attr)
        d.vbo   = VertexBuffer<V>(device, obj.vbo, attr);
        for (field<typename Obj::group> &fgroup: obj.groups) {
            symbol       name = fgroup.key;
            typename Obj::group &group = fgroup.value;
            auto     ibo = IndexBuffer<uint32_t>(device, group.ibo);
            str   s_name = name;
            d.part[name] = Pipeline<V>(device, ubo, d.vbo, ibo, assets, rgba {0.0, 0.0, 0.0, 0.0}, shaders(s_name));
        }
    }
};

struct font:mx {
    struct data {
        str  alias;
        real sz;
        path res;
    } &info;

    ctr(font, mx, data, info);
    font(str alias, real sz, path res) : font(data { alias, sz, res }) { }

    /// if there is no res, it can use font-config; there must always be an alias set, just so any cache has identity
    static font default_font() {
        static font def; ///
        if        (!def) ///
                    def = font("monaco", 16, "fonts/monaco.ttf");
        return      def; ///
    }
            operator bool()    { return  info.alias; }
    bool    operator!()        { return !info.alias; }
};

///
struct glyph:mx {
    ///
    struct members {
        int        border;
        str        chr;
        rgba::data bg;
        rgba::data fg;
    } &m;

    ctr(glyph, mx, members, m);

    str ansi();
    
    bool operator==(glyph &lhs) {
        return   (m.border == lhs->border) && (m.chr == lhs->chr) &&
                 (m.bg     == lhs->bg)     && (m.fg  == lhs->fg);
    }

    bool operator!=(glyph &lhs) { return !operator==(lhs); }
};

struct cbase:mx {
    struct draw_state {
        real             outline_sz;
        real             font_sz;
        real             line_height;
        real             opacity;
        graphics::shape  clip;
        str              font;
        m44              mat44;
        rgba             color;
        vec2             blur;
        graphics::cap    cap;
        graphics::join   join;
    };

    ///
    enums(state_change, defs,
       "defs, push, pop",
        defs, push, pop);
    
    struct cdata {
        size               size; /// size in integer units; certainly not part of the stack lol.
        type_t             pixel_t;
        doubly<draw_state> stack; /// ds = states.last()
        draw_state*        state; /// todo: update w push and pop
    } &m;

    ctr(cbase, mx, cdata, m);

    protected:
    virtual void init() { }
    
    draw_state &cur() {
        if (m.stack.len() == 0)
            m.stack += draw_state(); /// this errors in graphics::cap initialize (type construction, Deallocate error indicates stack corruption?)
        
        return m.stack.last();
    }

    public:
  ::size &size() { return m.size; }

    virtual void    outline(graphics::shape) { }
    virtual void       fill(graphics::shape) { }
    virtual void       clip(graphics::shape cl) {
        draw_state  &ds = cur();
        ds.clip = cl;
    }

    virtual void      flush() { }
    virtual void       text(str, graphics::shape, vec2, vec2, bool) { }
    virtual void      image(::image, graphics::shape, vec2, vec2)   { }
    virtual void    texture(::image)           { }
    virtual void      clear()                  { }
    virtual void      clear(rgba)              { }
    virtual tm_t    measure(str s)               { assert(false); return {}; }
    virtual void        cap(graphics::cap   cap) { cur().cap     = cap;      }
    virtual void       join(graphics::join join) { cur().join    = join;     }
    virtual void    opacity(real        opacity) { cur().opacity = opacity;  }
    virtual void       font(::font f)          { }
    virtual void      color(rgba)              { }
    virtual void   gaussian(vec2, graphics::shape) { }
    virtual void      scale(vec2)              { }
    virtual void     rotate(real)              { }
    virtual void  translate(vec2)              { }
    virtual void   set_char(int, int, glyph)   { } /// this needs to be tied to pixel unit (cbase 2nd T arg)
    virtual str    get_char(int, int)          { return "";    }
    virtual str  ansi_color(rgba &c, bool text) { return null;  }
    virtual ::image  resample(::size sz, real deg = 0.0f, graphics::rect view = null, vec2 rc = null) {
        return null;
    }

    virtual void draw_state_change(draw_state *ds, state_change sc) { }

    /// these two arent virtual but draw_state_changed_is; do all of the things there.
    void push() {
        m.stack +=  cur();
        m.state  = &cur();
        /// first time its pushed in more hacky dependency chain ways
        draw_state_change(m.state, state_change::push);
    }

    void  pop() {
        m.stack.pop();
        m.state = &cur();
        ///
        draw_state_change(m.state, state_change::pop);
    }

    void    outline_sz(real sz)  { cur().outline_sz  = sz;  }
    void    font_sz   (real sz)  { cur().font_sz     = sz;  }
    void    line_height(real lh) { cur().line_height = lh;  }
    m44     get_matrix()         { return cur().mat44;      }
    void    set_matrix(m44 m)    {        cur().mat44 = m;  }
    void      defaults()         {
        draw_state &ds = cur();
        ds.line_height = 1; /// its interesting to base units on a font-derrived line-height
                            /// in the case of console context, this allows for integral rows and columns for more translatable views to context2d
    }
    void         *data();

    /// boolean operator
    inline operator bool() { return bool(m.size); }
};

struct gfx_memory;
template <> struct is_opaque<gfx_memory> : true_type { };

/// gfx: a frontend on skia
struct gfx:cbase {
    gfx_memory* g;
    
    /// create with a window (indicated by a name given first)
    gfx(::window &w);

    /// data is single instanced on this cbase, and the draw_state is passed in as type for the cbase, which atleast makes it type-unique
    ptr_decl(gfx, cbase, gfx_memory, g);

    ::window       &window();
    Device         &device();
    void draw_state_change(draw_state *ds, cbase::state_change type);
    text_metrics   measure(str text);
    str    format_ellipsis(str text, real w, text_metrics &tm_result);
    void     draw_ellipsis(str text, real w);
    void             image(::image img, graphics::shape sh, vec2 align, vec2 offset, vec2 source);
    void              push();
    void               pop();
    void              text(str text, graphics::rect rect, alignment align, vec2 offset, bool ellip);
    void              clip(graphics::shape cl);
    Texture        texture();
    void             flush();
    void             clear(rgba c);
    void              font(::font f);
    void               cap(graphics::cap   c);
    void              join(graphics::join  j);
    void         translate(vec2       tr);
    void             scale(vec2       sc);
    void             scale(real       sc);
    void            rotate(real     degs);
    void              fill(graphics::shape  p);
    void          gaussian(vec2 sz, graphics::shape c);
    void           outline(graphics::shape p);
    void*             data();
    str           get_char(int x, int y);
    str         ansi_color(rgba &c, bool text);
    ::image       resample(::size sz, real deg, graphics::shape view, vec2 axis);
};

struct terminal:cbase {
    static inline symbol reset_chr = "\u001b[0m";

    protected:
    struct tdata {
        array<glyph> glyphs;
        draw_state  *ds;
    } &t;

    public:

    terminal(::size sz);
    ctr(terminal, cbase, tdata, t);

    void draw_state_change(draw_state &ds, cbase::state_change type);
    str         ansi_color(rgba &c, bool text);
    void             *data();
    void          set_char(int x, int y, glyph gl);
    str           get_char(int x, int y);
    void              text(str s, graphics::shape vrect, alignment::data &align, vec2 voffset, bool ellip);
    void           outline(graphics::shape sh);
    void              fill(graphics::shape sh);
};

enums(operation, fill,
    "fill, image, outline, text, child",
     fill, image, outline, text, child);

struct node:Element {
    /// standard props
    struct props {
        struct events {
            dispatch        hover;
            dispatch        out;
            dispatch        down;
            dispatch        up;
            dispatch        key;
            dispatch        focus;
            dispatch        blur;
            dispatch        cursor;
        } ev;

        struct drawing {
            rgba                color; 
            real                offset;
            alignment           align; 
            image               img;   
            region              area;  
            graphics::shape     shape; 
            graphics::cap       cap;   
            graphics::join      join;
            graphics::border    border;
            inline graphics::rect rect() { return shape.rect(); }
        };

        /// this is simply bounds, not rounded
        inline graphics::shape shape() {
            return drawings[operation::child].shape;
        }
        inline graphics::rect bounds() {
            return graphics::rect { shape() };
        }

        ///
        str                 id;         ///
        drawing             drawings[operation::count];
        graphics::rect      container;  /// the parent child rectangle
        int                 tab_index;  /// if 0, this is default; its all relative numbers we dont play absolutes.
        states<interaction> istates;    /// interaction states; these are referenced in qualifiers
        vec2                cursor;     /// set relative to the area origin
        vec2                scroll = {0,0};
        std::queue<fn_t>    queue;      /// this is an animation queue
        bool                active;
        bool                focused;
        type_t              type;       /// this is the type of node, as stored by the initializer of this data
        mx                  content;
        mx                  bind;       /// bind is useful to be in mx form, as any object can be key then.
    } &std;

    /// type and prop name lookup in tree
    /// there can also be an mx-based context.  it must assert that the type looked up in meta_map inherits from mx.  from there it can be ref memory,
    template <typename T>
    T &context(memory *sym) {
        node  *cur = (node*)this;
        while (cur) {
            type_t ctx = cur->mem->type;
            if (ctx) {
                prop *def = ctx->schema->meta_map.lookup((symbol&)*sym); /// will always need schema
                if (def) {
                    T &ref = def->member_ref<T, props>(cur->std); // require inheritance here
                    return ref;
                }
            }
            cur = cur->e.parent;
        }
        assert(false);
        T *n = null;
        return *n;
    }

    /// base drawing routine, easiest to understand and i think favoured over a non-virtual that purely uses the meta tables in polymorphic back tracking fashion
    /// one could lazy-cache that but i dunno.  why make drawing things complicated and covered in lambdas.  during iteration in debug you thank thy-self
    virtual void draw(gfx& canvas) {
        props::drawing &fill    = std.drawings[operation::fill];
        props::drawing &image   = std.drawings[operation::image];
        props::drawing &text    = std.drawings[operation::text];
        props::drawing &outline = std.drawings[operation::outline]; /// outline is more AR than border.  and border is a bad idea, badly defined and badly understood. outline is on the 0 pt.  just offset it if you want.
        
        /// if there is a fill color
        if (fill.color) { /// free'd prematurely during style change (not a transition)
            canvas.color(fill.color);
            canvas.fill(fill.shape);
        }

        /// if there is fill image
        if (image.img)
            canvas.image(image.img, image.shape, image.align, {0,0}, {0,0});
        
        /// if there is text (its not alpha 0, and there is text)
        if (std.content && std.content.type() == typeof(char) ||
                           std.content.type() == typeof(str)) {
            canvas.color(text.color);
            canvas.text(
                std.content.grab(), text.shape.rect(),
                text.align, {0.0, 0.0}, true);
        }

        /// if there is an effective border to draw
        if (outline.color && outline.border->size > 0.0) {
            canvas.color(outline.border->color);
            canvas.outline_sz(outline.border->size);
            canvas.outline(outline.shape); /// this needs to work with vshape, or border
        }
    }

    vec2 offset() {
        node *n = e.parent;
        vec2  o = { 0, 0 };
        while (n) {
            props::drawing &draw = n->std.drawings[operation::child];
            r4<real> &rect = draw.shape.rect();
            o  += rect.xy();
            o  -= n->std.scroll;
            n   = n->e.parent;
        }
        return o;
    }

    array<node*> select(lambda<node*(node*)> fn) {
        array<node*> result;
        lambda<node *(node *)> recur;
        recur = [&](node* n) -> node* {
            node* r = fn(n);
            if   (r) result += r;
            /// go through mount fields mx(symbol) -> node*
            for (field<node*> &f:n->Element::e.mounts)
                if (f.value) recur(f.value);
            return null;
        };
        recur(this);
        return result;
    }

    // node(Element::data&); /// constructs default members.  i dont see an import use-case for members just yet
    ctr(node, Element, props, std);

    node(type_t type, initial<arg> args) : Element(type, args), std(defaults<props>()) { }

    node *root() const {
        node  *pe = (node*)this;
        while (pe) {
            if (!pe->e.parent) return pe;
            pe = pe->e.parent;
        }
        return null;
    }

    style *fetch_style() const { return  root()->e.root_style; } /// fetch data from Element's accessor
    Device        *dev() const {
        return null;
    }

    doubly<prop> meta() const {
        return {
            prop { std, "id",        std.id },
            prop { std, "on-hover",  std.ev.hover  }, /// must give member-parent-origin (std) to be able to 're-apply' somewhere else
            prop { std, "on-out",    std.ev.out    },
            prop { std, "on-down",   std.ev.down   },
            prop { std, "on-up",     std.ev.up     },
            prop { std, "on-key",    std.ev.key    },
            prop { std, "on-focus",  std.ev.focus  },
            prop { std, "on-blur",   std.ev.blur   },
            prop { std, "on-cursor", std.ev.cursor },
            prop { std, "on-hover",  std.ev.hover  },
            prop { std, "content",   std.content   }
        };
    }

    inline size_t count(memory *symbol) {
        for (Element &c: *e.children)
            if (c.e.id == symbol)
                return 1;
        ///
        return 0;
    }

    node *select_first(lambda<node*(node*)> fn) {
        lambda<node*(node*)> recur;
        recur = [&](node* n) -> node* {
            node* r = fn(n);
            if   (r) return r;
            for (field<node*> &f: n->Element::e.mounts) {
                if (!f.value) continue;
                node* r = recur(f.value);
                if   (r) return r;
            }
            return null;
        };
        return recur(this);
    }

    void exec(lambda<void(node*)> fn) {
        lambda<node*(node*)> recur;
        recur = [&](node* n) -> node* {
            fn(n);
            for (field<node*> &f: n->Element::e.mounts)
                if (f.value) recur(f.value);
            return null;
        };
        recur(this);
    }

    /// test this path in compositor
    virtual Element update() {
        return *this;
    }
};

struct style:mx {
    /// construct with code
    style(str  code);
    style(path css) : style(css.read<str>()) { }

    struct qualifier:mx {
        struct members {
            str       type;
            str       id;
            str       state; /// member to perform operation on (boolean, if not specified)
            str       oper;  /// if specified, use non-boolean operator
            str       value;
        } &data;
        ctr(qualifier, mx, members, data);
    };

    ///
    struct transition:mx {
        enums(curve, none, 
            "none, linear, ease-in, ease-out, cubic-in, cubic-out",
             none, linear, ease_in, ease_out, cubic_in, cubic_out);
        
        struct members {
            curve ct; /// curve-type, or counter-terrorist.
            scalar<nil, duration> dur;
        } &data;

        ctr(transition, mx, members, data);

        inline real pos(real tf) const {
            real x = math::clamp<real>(tf, 0.0, 1.0);
            switch (data.ct) {
                case curve::none:      x = 1.0;                    break;
                case curve::linear:                                break; // /*x = x*/  
                case curve::ease_in:   x = x * x * x;              break;
                case curve::ease_out:  x = x * x * x;              break;
                case curve::cubic_in:  x = x * x * x;              break;
                case curve::cubic_out: x = 1 - std::pow((1-x), 3); break;
            };
            return x;
        }

        template <typename T>
        inline T operator()(T &fr, T &to, real value) const {
            constexpr bool transitions = transitionable<T>();
            if constexpr (transitions) {
                real x = pos(value);
                real i = 1.0 - x;
                return (fr * i) + (to * x);
            } else
                return to;
        }

        transition(str s) : transition() {
            if (s) {
                array<str> sp = s.split();
                size_t    len = sp.len();
                ///
                if (len == 2) {
                    /// 0.5s ease-out [things like that]
                    data.dur = sp[0];
                    data.ct  = curve(sp[1]).value;
                } else if (len == 1) {
                    doubly<memory*> &sym = curve::symbols();
                    if (s == str(sym[0])) {
                        /// none
                        data.ct  = curve::none;
                    } else {
                        /// 0.2s
                        data.dur = sp[0];
                        data.ct  = curve::linear; /// linear is set if any duration is set; none = no transition
                    }
                } else
                    console.fault("transition values: none, count type, count (seconds default)");
            }
        }
        ///
        operator  bool() { return data.ct != curve::none; }
        bool operator!() { return data.ct == curve::none; }
    };

    /// Element or style block entry
    struct entry:mx {
        struct data {
            mx              member;
            str             value;
            transition      trans;
        } &m;
        ctr(entry, mx, data, m);
    };

    ///
    struct block:mx {
        struct data {
            mx                       parent; /// pattern: reduce type rather than create pointer to same type in delegation
            doubly<style::qualifier> quals;  /// an array of qualifiers it > could > be > this:state, or > just > that [it picks the best score, moves up for a given node to match style in]
            doubly<style::entry>     entries;
            doubly<style::block>     blocks;
        } &m;
        
        ///
        ctr(block, mx, data, m);
        
        ///
        inline size_t count(str s) {
            for (entry &p:m.entries)
                if (p->member == s)
                    return 1;
            return 0;
        }

        ///
        inline entry *b_entry(mx member_name) {
            for (entry &p:m.entries)
                if (p->member == member_name)
                    return &p;
            return null;
        }

        size_t score(node *n) {
            double best_sc = 0;
            for (qualifier &q:m.quals) {
                qualifier::members &qd = q.data;
                bool    id_match  = qd.id    &&  qd.id == n->e.id;
                bool   id_reject  = qd.id    && !id_match;
                bool  type_match  = qd.type  &&  strcmp((symbol)qd.type.cs(), (symbol)n->mem->type->name) == 0; /// class names are actual type names
                bool type_reject  = qd.type  && !type_match;
                bool state_match  = qd.state &&  n->e.istates[qd.state];
                bool state_reject = qd.state && !state_match;
                ///
                if (!id_reject && !type_reject && !state_reject) {
                    double sc = size_t(   id_match) << 1 |
                                size_t( type_match) << 0 |
                                size_t(state_match) << 2;
                    best_sc = math::max(sc, best_sc);
                }
            }
            return best_sc;
        };

        /// each qualifier is defined as it, and all of the blocked qualifiers below.
        /// there are more optimal data structures to use for this matching routine
        /// state > state2 would be a nifty one, no operation indicates bool, as-is current normal syntax
        double match(node *from) {
            block          &bl = *this;
            double total_score = 0;
            int            div = 0;
            node            *n = from;
            ///
            while (bl && n) {
                bool   is_root   = !bl->parent;
                double score     = 0;
                ///
                while (n) { ///
                    auto sc = bl.score(n);
                    n       = n->e.parent;
                    if (sc == 0 && is_root) {
                        score = 0;
                        break;
                    } else if (sc > 0) {
                        score += double(sc) / ++div;
                        break;
                    }
                }
                total_score += score;
                ///
                if (score > 0) {
                    /// proceed...
                    bl = block(bl->parent);
                } else {
                    /// style not matched
                    bl = null;
                    total_score = 0;
                }
            }
            return total_score;
        }

        ///
        inline operator bool() { return m.quals || m.entries || m.blocks; }
    };

    static inline map<style> cache;

    struct data {
        array<block>      root;
        map<array<block>> members;
    } &m;

    ctr(style, mx, data, m);

    array<block> &members(mx &s_member) {
        return m.members[s_member];
    }

    ///
    void cache_members();

    /// load .css in style res
    static void init(path res = "style") {
        res.resources({".css"}, {},
            [&](path css_file) -> void {
                for_class(css_file.cs());
            });
    }

    static style load(path p);
    static style for_class(symbol);
};

style::entry *prop_style(node &n, prop *member);

//typedef node* (*FnFactory)();
using FnFactory = node*(*)();

/// a type registered enum, with a default value
enums(rendition, none,
    "none, shader, wireframe",
     none, shader, wireframe);

using construction = Pipes; /// generic of Model<T> ; will call it this instead of Pipes lol.
using AssetUtil    = array<Asset>;

template <typename V>
struct object:node {
    /// our members
    struct members {
        protected:
            construction    plumbing;
        public:
            str             model     = "";
            str             skin      = "";
            states<Asset>   assets    = { Asset::Color };
            Shaders         shaders   = { "*=main" };
            UniformData     ubo       = { null };
            VAttribs        attr      = { VA::Position, VA::UV, VA::Normal };
            rendition       render    = { rendition::shader };
    } &m;

    /// make a node_constructors
    ctr(object, node, members, m)
    
    /// our interface
    doubly<prop> meta() {
        return {
            prop { m, "model",     m.model },
            prop { m, "skin",      m.skin },
            prop { m, "assets",    m.assets },
            prop { m, "shaders",   m.shaders },
            prop { m, "ubo",       m.ubo },
            prop { m, "attr",      m.attr },
            prop { m, "render",    m.render }
        };
    }
    
    /// change management, we probably have to give prev values in a map.
    void changed(doubly<prop> list) {
        m.plumbing = Model<Vertex>(m.model, m.skin, m.ubo, m.attr, m.assets, m.shaders);
    }
    
    /// rendition of pipes
    Element update() {
        if (m.plumbing)
            for (auto &[pipe, name]: m.plumbing.map())
                dev()->push(pipe);
        return node::update();
    }
};

///
struct button:node {
    enums(behavior, push,
        "push, label, toggle, radio",
         push, label, toggle, radio);
    ///
    struct props {
        behavior behavior;
    } &m;
    
    ///
    ctr_args(button, node, props, m);
};

#if 0

/// bind-compatible List component for displaying data from models or static (type cases: array)
struct list_view:node {
    struct Column:mx {
        struct data {
            str     id     = null;
            real    final  = 0;
            real    value  = 1.0;
            bool    scale  = true;
            xalign  align  = xalign::left;
        } &a;
        
        ctr(Column, mx, data, a);

        Column(str id, real scale = 1.0, xalign ax = xalign::left) : Column() {
            a.id    = id;
            a.value = scale;
            a.scale = true;
            a.align = ax;
        }
        
        Column(str id, int size, xalign ax = xalign::left) : Column() {
            a.id    = id;
            a.value = size;
            a.scale = false;
            a.align = ax;
        }
    };
    
    array<Column> columns;
    using Columns = array<Column>;
    
    struct Members {
        ///
        struct Cell {
            mx        id;
            str       text;
            rgba      fg;
            rgba      bg;
            alignment va;
        } cell;
        
        struct ColumnConf:Cell {
            Columns   ids;
        } column;

        rgba odd_bg;
    } m;
    
    doubly<prop> meta() {
        return {
            prop { m, "cell-fg",    m.cell.fg    }, /// designer gets to set app-defaults in css; designer gets to know entire component model; designer almost immediately regrets
            prop { m, "cell-fg",    m.cell.fg    },
            prop { m, "cell-bg",    m.cell.bg    },
            prop { m, "odd-bg",     m.odd_bg     },
            prop { m, "column-fg",  m.column.fg  },
            prop { m, "column-bg",  m.column.bg  },
            prop { m, "column-ids", m.column.ids }
        };
    }
    
    void update_columns() {
        columns       = array<Column>(m.column.ids);
        double  tsz   = 0;
        double  warea = node::std.drawings[operation::fill].shape.w();
        double  t     = 0;
        
        for (Column::data &c: columns)
            if (c.scale)
                tsz += c.value;
        ///
        for (Column::data &c: columns)
            if (c.scale) {
                c.value /= tsz;
                t += c.value;
            } else
                warea = math::max(0.0, warea - c.value);
        ///
        for (Column::data &c: columns)
            if (c.scale)
                c.final = math::round(warea * (c.value / t));
            else
                c.final = c.value;
    }
    
    void draw(gfx &canvas) {
        update_columns();
        node::draw(canvas);

        members::drawing &fill    = std.drawings[operation::fill];
        members::drawing &image   = std.drawings[operation::image];
        members::drawing &text    = std.drawings[operation::text];
        members::drawing &outline = std.drawings[operation::outline]; /// outline is more AR than border.  and border is a bad idea, badly defined and badly understood. outline is on the 0 pt.  just offset it if you want.
        

        /// standard bind (mx) is just a context lookup, key to value in app controller; filters applied during render
        mx data = context(node::std.bind);
        if (!data) return; /// use any smooth bool operator

        ///
        r4<real> &rect   = node::std.drawings[operation::fill].rect();
        r4<real>  h_line = { rect.x, rect.y, rect.w, 1.0 };

        /// would be nice to just draw 1 outline here, canvas should work with that.
        auto draw_rect = [&](rgba &c, r4<real> &h_line) {
            rectr  r   = h_line;
            shape vs  = r; /// todo: decide on final xy scaling for text and gfx; we need a the same unit scales
            canvas.push();
            canvas.color(c);
            canvas.outline(vs);
            canvas.pop();
        };
        Column  nc = null;
        auto cells = [&] (lambda<void(Column::data &, r4<real> &)> fn) {
            if (columns) {
                v2d p = h_line.xy();
                for (Column::data &c: columns) {
                    double w = c.final;
                    r4<real> cell = { p.x, p.y, w - 1.0, 1.0 };
                    fn(c, cell);
                    p.x += w;
                }
            } else
                fn(nc, h_line);
        };

        ///
        auto pad_rect = [&](r4<real> r) -> r4<real> {
            return {r.x + 1.0, r.y, r.w - 2.0, r.h};
        };
        
        /// paint column text, fills and outlines
        if (columns) {
            bool prev = false;
            cells([&](Column::data &c, r4<real> &cell) {
                //var d_id = c.id;
                str label = "n/a";//context("resolve")(d_id); -- c
                canvas.color(text.color);
                canvas.text(label, pad_rect(cell), { xalign::middle, yalign::middle }, { 0, 0 }, true);
                if (prev) {
                    r4<real> cr = cr;
                    draw_rect(outline.border.color(), cr);
                } else
                    prev = true;
            });
            r4<real> rr = { h_line.x - 1.0, h_line.y + 1.0, h_line.w + 2.0, 0.0 }; /// needs to be in props
            draw_rect(outline.border.color(), rr);
            h_line.y += 2.0; /// this too..
        }
        
        /// paint visible rows
        array<mx> d_array(data.grab());
        double  sy = std.scroll.data.y;
        int  limit = sy + fill.h() - (columns ? 2.0 : 0.0);
        for (int i = sy; i < math::min(limit, int(d_array.len())); i++) {
            mx &d = d_array[i];
            cells([&](Column::data &c, r4<real> &cell) {
                str s = c ? str(d[c.id]) : str(d);
                rgba &clr = (i & 1) ? m.odd_bg : m.cell.bg;
                canvas.color(clr);
                canvas.fill(cell);
                canvas.color(text.color);
                canvas.text(s, pad_rect(cell), { xalign::left, yalign::middle }, {0,0});
            });
            r.y += r.h;
        }
    }
};
#endif

struct composer:mx {
    ///
    struct data {
        node         *root;
        struct vk_interface *vk;
        //fn_render     render;
        lambda<Element()> render;
        map<mx>       args;
    } &d;
    ///
    ctr(composer, mx, data, d);
    ///
    array<node *> select_at(vec2 cur, bool active = true) {
        array<node*> inside = d.root->select([&](node *n) {
            real      x = cur.data.x, y = cur.data.y;
            vec2      o = n->offset();
            vec2    rel = cur.data + o.data;

            /// it could be Element::drawing
            node::props::drawing &draw = n->std.drawings[operation::fill];

            bool     in = draw.shape.contains(rel);
            n->std.cursor = in ? vec2(x, y) : vec2(null);
            return (in && (!active || !n->std.active)) ? n : null;
        });

        array<node*> actives = d.root->select([&](node *n) -> node* {
            return (active && n->std.active) ? n : null;
        });

        array<node*> result = array<node *>(size_t(inside.len()) + size_t(actives.len()));

        for (node *i: inside)
            result += i;
        for (node *i: actives)
            result += i;
        
        return result;
    }
    ///
};

struct app:composer {
    struct data {
        window win;
        gfx    canvas;
        lambda<Element(app&)> app_fn;
    } &m;

    ctr(app, composer, data, m);

    app(lambda<Element(app&)> app_fn) : app() {
        m.app_fn = app_fn;
    }
    ///
    int run();

    operator int() { return run(); }
};

using AppFn = lambda<Element(app&)>;
