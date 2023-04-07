module;
#include <core/core.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vkvg.h>
#include <array>
#include <set>
#include <stack>
#include <queue>
#include <vkh.h>
#include <vulkan/vulkan.h>

export module ux;

import core;
import vector;
import async;
import image;

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
export namespace graphics {
    export enums(cap, none,
       "none, blunt, round",
        none, blunt, round);
    
    export enums(join, miter,
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
        
        void vkvg_path(VkvgContext ctx) {
            data &m = ref<data>();
            vkvg_move_to (ctx, m.tl_x.x, m.tl_x.y);
            vkvg_line_to (ctx, m.tr_x.x, m.tr_x.y);
            vkvg_curve_to(ctx, m.c0.x,   m.c0.y, m.c1.x, m.c1.y, m.tr_y.x, m.tr_y.y);
            vkvg_line_to (ctx, m.br_y.x, m.br_y.y);
            vkvg_curve_to(ctx, m.c0b.x,  m.c0b.y, m.c1b.x, m.c1b.y, m.br_x.x, m.br_x.y);
            vkvg_line_to (ctx, m.bl_x.x, m.bl_x.y);
            vkvg_curve_to(ctx, m.c0c.x,  m.c0c.y, m.c1c.x, m.c1c.y, m.bl_y.x, m.bl_y.y);
            vkvg_line_to (ctx, m.tl_y.x, m.tl_y.y);
            vkvg_curve_to(ctx, m.c0d.x,  m.c0d.y, m.c1d.x, m.c1d.y, m.tl_x.x, m.tl_x.y);
        }
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

        // handle rects
        void vkvg_path(VkvgContext ctx) {
            for (mx &o:m.ops) {
                type_t t = o.type();
                if (t == typeof(movement)) {
                    movement m(o);
                    vkvg_move_to(ctx, m.data.x, m.data.y); /// todo: convert vkvg to double.  simply not using float!
                } else if (t == typeof(vec2)) {
                    vec2 l(o);
                    vkvg_line_to(ctx, l.data.x, l.data.y);
                }
            }
            vkvg_close_path(ctx);
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
    
    /// graphics::border is an indication that this has information on color
    struct border:mx {
        struct data {
            real sz, tl, tr, bl, br;
            rgba color;
            inline bool operator==(const data &b) const { return b.tl == tl && b.tr == tr && b.bl == bl && b.br == br; }
            inline bool operator!=(const data &b) const { return !operator==(b); }
        } &m;
        ///
        ctr(border, mx, data, m);
        ///
        border(str raw) : border() {
            str        trimmed = raw.trim();
            size_t     tlen    = trimmed.len();
            array<str> values  = raw.split();
            size_t     ncomps  = values.len();
            
            if (tlen > 0) {
                m.sz = values[0].real_value<real>();
                if (ncomps == 5) {
                    m.tl = values[1].real_value<real>();
                    m.tr = values[2].real_value<real>();
                    m.br = values[3].real_value<real>();
                    m.bl = values[4].real_value<real>();
                } else if (tlen > 0 && ncomps == 2)
                    m.tl = m.tr = m.bl = m.br = values[1].real_value<real>();
                else
                    console.fault("border requires 1 (size) or 2 (size, roundness) or 5 (size, tl tr br bl) values");
            }
        }

        inline real & size() { return m.sz; }
        inline rgba &color() { return m.color; }
    };

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
        return origin + m.is_percent ? (m.scale / 100.0 * size) : m.scale;
    }

    inline bool operator>=(P::etype p) const { return p >= m.prefix; }
    inline bool operator> (P::etype p) const { return p >  m.prefix; }
    inline bool operator<=(P::etype p) const { return p <= m.prefix; }
    inline bool operator< (P::etype p) const { return p <  m.prefix; }
    inline bool operator>=(S::etype s) const { return s >= m.suffix; }
    inline bool operator> (S::etype s) const { return s >  m.suffix; }
    inline bool operator<=(S::etype s) const { return s <= m.suffix; }
    inline bool operator< (S::etype s) const { return s <  m.suffix; }
    inline bool operator==(P::etype p) const { return p == m.prefix; }
    inline bool operator==(S::etype s) const { return s == m.suffix; }
    inline bool operator!=(P::etype p) const { return p != m.prefix; }
    inline bool operator!=(S::etype s) const { return s != m.suffix; }

    inline operator    P() const { return P::etype(m.prefix); }
    inline operator    S() const { return S::etype(m.suffix); }
    inline operator real() const { return m.scale; }

    scalar(P::etype prefix, real scale, S::etype suffix) : scalar(data { prefix, scale, suffix, false }) { }

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

export namespace user {
    struct chr:mx {
        struct data {
            num               unicode; /// num is a 64bit signed integer
        } &m;
        ctr(chr, mx, data, m);
    };

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

    struct cursor:mx {
        struct data {
            vec2 pos;
            states<mouse> buttons;
        } &m;
        ctr(cursor, mx, data, m);
    };

    struct resize:mx {
        struct data {
            size sz;
        } &m;
        ctr(resize, mx, data, m);
    };
};

export struct event:mx {
    ///
    struct edata {
        user::chr     chr;
        user::key     key;
        user::cursor  cursor;
        user::resize  resize;
        states<mouse> buttons;
        bool          prevent_default;
        bool          stop_propagation;
    } &evd;

    ///
    ctr(event, mx, edata, evd);
    
    ///
    inline void prevent_default()   {         evd.prevent_default = true;  }
    inline bool is_default()        { return !evd.prevent_default;         }
    inline bool should_propagate()  { return !evd.stop_propagation;        }
    inline bool stop_propagation()  { return  evd.stop_propagation = true; }
    inline vec2 cursor_pos()        { return  evd.cursor->pos;            }
    inline bool mouse_down(mouse m) { return  evd.cursor->buttons[m];     }
    inline bool mouse_up  (mouse m) { return !evd.cursor->buttons[m];     }
    inline num  unicode   ()        { return  evd.key->unicode;           }
    inline num  scan_code ()        { return  evd.key->scan_code;         }
    inline bool key_down  (num u)   { return  evd.key->unicode   == u && !evd.key->up; }
    inline bool key_up    (num u)   { return  evd.key->unicode   == u &&  evd.key->up; }
    inline bool scan_down (num s)   { return  evd.key->scan_code == s && !evd.key->up; }
    inline bool scan_up   (num s)   { return  evd.key->scan_code == s &&  evd.key->up; }
};


struct node;
struct style;

/// enumerables for the state bits
export enums(interaction, undefined,
    "undefined, captured, focused, hover, active, cursor",
     undefined, captured, focused, hover, active, cursor);

export struct   map_results:mx {   map_results():mx() { } };
export struct array_results:mx { array_results():mx() { } };

export struct Element:mx {
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
        data { type, null, null, new array<Element>(sz, 0) }
    ) { }

    template <typename T>
    static Element each(array<T> a, lambda<Element(T &v)> fn) {
        Element res(typeof(map_results), a.len());
        if (e.children)
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
    void operator()(event e) {
        for (listener &l: m.listeners)
            l->cb(e);
    }
    ///
    listener &listen(callback cb) {
        listener  ls   = listener({ this, cb });
        listener &last = m.listeners.last();

        last->detatch = fn_stub([ls_mem=last.mem, listeners=&m.listeners]() -> void {
            size_t i = 0;
            bool found = false;
            for (listener &ls: *listeners) {
                if (ls.mem == ls_mem) {
                    found = true;
                    break;
                }
                i++;
            }
            console.test(found);
            listeners->remove(num(i)); 
        });

        return m.listeners += ls;
    }
};

struct Device;

VkDevice device_handle(Device *device);
VkPhysicalDevice gpu_handle(Device *device);
uint32_t memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props);
symbol cstr_copy   (symbol s);
void   glfw_error  (int code, symbol cs);
void   glfw_key    (GLFWwindow *h, int key, int scancode, int action, int mods);
void   glfw_char   (GLFWwindow *h, u32 code);
void   glfw_button (GLFWwindow *h, int button, int action, int mods);
void   glfw_cursor (GLFWwindow *handle, double x, double y);
void   glfw_resize (GLFWwindow *handle, i32 w, i32 h);

symbol cstr_copy(symbol s) {
    size_t len = strlen(s);
    cstr     r = (cstr)malloc(len + 1);
    std::memcpy((void *)r, (void *)s, len + 1);
    return symbol(r);
}

void glfw_error(int code, symbol cstr) {
    console.log("glfw error: {0}", { str(cstr) });
}

struct Texture;

using Path    = path;
using Image   = image;
using strings = array<str>;
using Assets  = map<Texture *>;

/// simple wavefront obj
template <typename V>
struct obj:mx {
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

    ctr(obj, mx, members, m);

    obj(path p, lambda<V(group&, vec3&, vec2&, vec3&)> fn) : obj() {
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
            else if (w[0] == "v")  v  += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "vt") vt += vec2 { w[1].real(), w[2].real() };
            else if (w[0] == "vn") vn += vec3 { w[1].real(), w[2].real(), w[3].real() };
            else if (w[0] == "f") {
                assert(g.len());
                for (size_t i = 1; i < 4; i++) {
                    auto key = w[i];
                    if (indices.count(key) == 0) {
                        indices[key] = u32(verts++);
                        auto      sp =  w[i].split("/");
                        int      iv  = sp[0].integer();
                        int      ivt = sp[1].integer();
                        int      ivn = sp[2].integer();
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

struct GPU;
struct Texture;

struct window:mx {
    struct data {
        size             sz;
        bool             headless;
        memory          *control;
        str              title;
        event            state;
        Device          *dev;
        GPU             *gpu;
        GLFWwindow      *glfw;
        VkSurfaceKHR     vk_surface; 
        Texture         *tx_skia;
        vec2             dpi_scale = { 1, 1 };
        bool             repaint;
        states<mouse>    buttons;
        states<keyboard> modifiers;
        vec2             cursor;
        lambda<void()>   fn_resize;

        struct dispatchers {
            dispatch on_resize;
            dispatch on_char;
            dispatch on_button;
            dispatch on_cursor;
            dispatch on_key;
        } events;
        
        /// window destructor, when the memory loses all refs is when you destroy the resources
        /// any sort of passing around just results in an mx construction, ref increase
        ~data();
        
    } &w;

    ctr(window, mx, data, w);

    Device   &   device();
    VkDevice &vk_device();

    Texture  &texture();
    Texture  &texture(::size sz);
    
    void loop(lambda<void()> fn) {
        while (!glfwWindowShouldClose(w.glfw)) {
            fn();
            glfwPollEvents();
        }
    }

    window(size sz, mode::etype m, memory *control);

    inline static window handle(GLFWwindow *h) {
        return ((memory *)glfwGetWindowUserPointer(h))->grab();
    }

    inline void button_event(states<mouse> mouse_state) {
        w.events.on_button(mouse_state);
    }

    inline void char_event(num unicode) {
        w.events.on_char(
            event(event::edata { .chr = unicode })
        );
    }

    inline void wheel_event(vec2 wheel_delta) {
        w.events.on_cursor( wheel_delta );
    }

    inline void key_event  (user::key::data key) {
        w.events.on_cursor(
            event(event::edata { .key = key })
        );
    }

    inline void cursor_event(user::cursor::data cursor) {
        w.events.on_cursor(
            event(event::edata { .cursor = cursor })
        );
    }

    inline void resize_event(user::resize::data resize) {
        w.events.on_resize(
            event(event::edata { .resize = resize })
        );
    }

    bool key(int key);

    vec2 cursor() {
        v2<real> result { 0, 0 };
        glfwGetCursorPos(w.glfw, &result.x, &result.y);
        return vec2(result);
    }

    str  clipboard    ()         { return glfwGetClipboardString(w.glfw); }
    void set_clipboard(str text) { glfwSetClipboardString(w.glfw, text.cs()); }

    void set_title    (str s)    {
        w.title = s;
        glfwSetWindowTitle(w.glfw, w.title.cs());
    }

    void              show() { glfwShowWindow(w.glfw); }
    void              hide() { glfwHideWindow(w.glfw); }
    void              start();

    VkPhysicalDevice& gpu();
    VkQueue&          queue();
    u32               graphics_index(); /// the index for graphics queue.  skia needs both and probably can store both VkQueue and index in tuple or something readable
    VkSurfaceKHR&     surface()  { return w.vk_surface; }
  ::size              size()     { return w.sz; }

    operator GLFWwindow*() { return      w.glfw;  }
    operator bool()        { return bool(w.glfw); }
};

struct GLFWwindow;

struct Device;
struct Texture;
struct Buffer {
    ///
    Device        *device;
    VkDescriptorBufferInfo info;
    VkBuffer       buffer;
    VkDeviceMemory memory;
    size_t         sz;
    size_t         type_size;
    void           destroy();
    operator       VkBuffer();
    operator       VkDeviceMemory();
    ///
    Buffer(std::nullptr_t n = null) : device(null) { }
    Buffer(Device *, size_t, VkBufferUsageFlags, VkMemoryPropertyFlags);
    
    template <typename T>
    Buffer(Device *d, array<T> &v, VkBufferUsageFlags usage, VkMemoryPropertyFlags mprops): sz(v.len() * sizeof(T)) {
        type_size = sizeof(T);
        VkDevice device = device_handle(d);
        VkBufferCreateInfo bi {};
        bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size         = VkDeviceSize(sz);
        bi.usage        = usage;
        bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

        assert(vkCreateBuffer(device, &bi, nullptr, &buffer) == VK_SUCCESS);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buffer, &req);
        
        /// allocate and bind
        VkMemoryAllocateInfo alloc { };
        alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize    = req.size;
        VkPhysicalDevice gpu    = gpu_handle(d);
        alloc.memoryTypeIndex   = memory_type(gpu, req.memoryTypeBits, mprops);
        assert(vkAllocateMemory(device, &alloc, nullptr, &memory) == VK_SUCCESS);
        vkBindBufferMemory(device, buffer, memory, 0);
        
        /// transfer memory
        assert(v.len() == sz / sizeof(T));
        void* data;
          vkMapMemory(device, memory, 0, sz, 0, &data);
               memcpy(data,   v.data(),  sz);
        vkUnmapMemory(device, memory);

        info        = VkDescriptorBufferInfo {};
        info.buffer = buffer;
        info.offset = 0;
        info.range  = sz; /// was:sizeof(UniformBufferObject)
    }
    
    void  copy_to(Buffer &, size_t);
    void  copy_to(Texture *);
    ///
    inline operator VkDescriptorBufferInfo &() {
        return info;
    }
};

///
struct IndexData {
    Device         *device = null;
    Buffer          buffer;
    operator VkBuffer() { return buffer; }
    size_t        len() { return buffer.sz / buffer.type_size; }
    IndexData(std::nullptr_t n = null) { }
    IndexData(Device &device, Buffer buffer) : device(&device), buffer(buffer)  { }
    operator bool () { return device != null; }
    bool operator!() { return device == null; }
};

///
template <typename I>
struct IndexBuffer:IndexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    IndexBuffer(std::nullptr_t n = null) : IndexData(n) { }
    IndexBuffer(Device &device, array<I> &i) : IndexData(device, Buffer {
            &device, i, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }) { }
};

/// vulkan people like to flush the entire glyph cache per type
typedef array<VkVertexInputAttributeDescription> VkAttribs;

///
struct VertexData {
    Device         *device = null;
    Buffer          buffer;
    std::function<VkAttribs()> fn_attribs;
    ///
    VertexData(std::nullptr_t n = null) { }
    VertexData(Device &device, Buffer buffer, std::function<VkAttribs()> fn_attribs) :
                device(&device), buffer(buffer), fn_attribs(fn_attribs)  { }
    ///
    operator VkBuffer() { return buffer;         }
    size_t       size() { return buffer.sz;      }
       operator bool () { return device != null; }
       bool operator!() { return device == null; }
};

struct Descriptor {
    Device               *device;
    VkDescriptorPool      pool;
    void     destroy();
    Descriptor(Device *device = null) : device(device) { }
};


struct Asset:ex {\
    inline static bool init;\
    enum etype { Undefined, Color, Normal, Specular, Displace };\
    enum etype&    value;\
    static memory* lookup(symbol sym) { return typeof(Asset)->lookup(sym); }\
    static memory* lookup(u64    id)  { return typeof(Asset)->lookup(id);  }\
    inline static const int count = num_args("Undefined, Color, Normal, Specular, Displace");\
    static void initialize() {\
        init            = true;\
        array<str>   sp = str("Undefined, Color, Normal, Specular, Displace").split(", ");\
        size_t        c = sp.len();\
        type_t       ty = typeof(Asset);\
        for (size_t i  = 0; i < c; i++)\
            mem_symbol(sp[i].data, ty, i64(i));\
    }\
    str name() {\
        memory *mem = typeof(Asset)->lookup(u64(value));\
        assert(mem);\
        return (char*)mem->origin;\
    }\
    static enum etype convert(mx raw) {\
        if (!init)\
            initialize();\
        type_t   type = typeof(Asset);\
        memory **psym = null;\
        if (raw.is_string()) {\
            char  *d = &raw.ref<char>();\
            u64 hash = djb2(d);\
            psym     = type->symbols->djb2.lookup(hash);\
        } else if (raw.type() == typeof(int)) {\
            i64   id = i64(raw.ref<int>());\
            psym     = type->symbols->ids.lookup(id);\
        } else if (raw.type() == typeof(i64)) {\
            i64   id = raw.ref<i64>();\
            psym     = type->symbols->ids.lookup(id);\
        }\
        if (!psym) throw Asset();\
        return (enum etype)((*psym)->id);\
    }\
    Asset(enum etype t = etype::Undefined):ex(t, this), value(ref<enum etype>()) { if (!init) initialize(); }\
    Asset(str raw):Asset(Asset::convert(raw)) { }\
    Asset(mx  raw):Asset(Asset::convert(raw)) { }\
    inline  operator        etype() { return value; }\
    static doubly<memory*> &symbols() {\
        if (!init) initialize();\
        return typeof(Asset)->symbols->list;\
    }\
    Asset&      operator=  (const Asset b)  { return (Asset&)assign_mx(*this, b); }\
    bool    operator== (enum etype v) { return value == v; }\
    bool    operator!= (enum etype v) { return value != v; }\
    bool    operator>  (Asset &b)       { return value >  b.value; }\
    bool    operator<  (Asset &b)       { return value <  b.value; }\
    bool    operator>= (Asset &b)       { return value >= b.value; }\
    bool    operator<= (Asset &b)       { return value <= b.value; }\
    explicit operator int()         { return int(value); }\
    explicit operator u64()         { return u64(value); }\
};\

//enums(Asset, Undefined,
//     "Undefined, Color, Normal, Specular, Displace",
//      Undefined, Color, Normal, Specular, Displace);

enums(VA, Position,
    "Position, Normal, UV, Color, Tangent, BiTangent",
     Position, Normal, UV, Color, Tangent, BiTangent);

using VAttribs = states<VA>;

VkDescriptorSetLayoutBinding Asset_descriptor(Asset::etype t) {
    return { uint32_t(t), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL, nullptr };
}

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
        struct Data {
            enum Type               value  = Undefined;
            VkImageLayout           layout = VK_IMAGE_LAYOUT_UNDEFINED;
            uint64_t                access = 0;
            VkPipelineStageFlagBits stage;
            inline bool operator==(Data &b) { return layout == b.layout; }
        };
        ///
        static const Data types[5];
        const Data & data() { return types[value]; }
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
    
    struct Data {
        Device                 *device     = null;
        VkImage                 vk_image   = VK_NULL_HANDLE;
        VkImageView             view       = VK_NULL_HANDLE;
        VkDeviceMemory          memory     = VK_NULL_HANDLE;
        VkDescriptorImageInfo   info       = {};
        VkSampler               sampler    = VK_NULL_HANDLE;
        size                    sz         = { 0, 0 };
        int                     mips       = 0;
        VkMemoryPropertyFlags   mprops     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkImageTiling           tiling     = VK_IMAGE_TILING_OPTIMAL; /// [not likely to be a parameter]
        bool                    ms         = false;
        bool                    image_ref  = false;
        VkFormat                format;
        VkImageUsageFlags       usage;
        VkImageAspectFlags      aflags;
        image                   lazy;
        ///
        std::stack<Stage>       stage;
        Stage                   prv_stage       = Stage::Undefined;
        VkImageLayout           layout_override = VK_IMAGE_LAYOUT_UNDEFINED;
        ///
        void                    set_stage(Stage);
        void                    push_stage(Stage);
        void                    pop_stage();
        static int              auto_mips(uint32_t, size &);
        ///
        static VkImageView      create_view(VkDevice, size &, VkImage, VkFormat, VkImageAspectFlags, uint32_t);
        ///
        operator                bool();
        bool                    operator!();
        operator                VkImage &();
        operator                VkImageView &();
        operator                VkDeviceMemory &();
        operator                VkAttachmentDescription();
        operator                VkAttachmentReference();
        operator                VkDescriptorImageInfo &();
        void                    destroy();
        VkImageView             image_view();
        VkSampler               image_sampler();
        void                    create_sampler();
        void                    create_resources();
        VkWriteDescriptorSet    descriptor(VkDescriptorSet &ds, uint32_t binding);
        ///
        Data(std::nullptr_t n = null) { }
        Data(Device *, size, rgba, VkImageUsageFlags,
             VkMemoryPropertyFlags, VkImageAspectFlags, bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1, Image = null);
        
        //device, im, usage, memory, aspect, ms, format, mips
        
        Data(Device *device,        Image &im,
             VkImageUsageFlags,     VkMemoryPropertyFlags,
             VkImageAspectFlags,    bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
        Data(Device *, size, VkImage, VkImageView, VkImageUsageFlags,
             VkMemoryPropertyFlags,    VkImageAspectFlags, bool,
             VkFormat = VK_FORMAT_R8G8B8A8_SRGB, int = -1);
    } &data;

    ctr(Texture, mx, Data, data);

    VkImage vk_image() { return data.vk_image; }

    Texture(Device            *device,  size sz,         rgba clr,
            VkImageUsageFlags  usage,   VkMemoryPropertyFlags memory,
            VkImageAspectFlags aspect,  bool                  ms,
            VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1, image lazy = null) :
        Texture {
            Data { device, sz, clr, usage, memory,
                   aspect, ms, format, mips, lazy }
        }
    {
        rgba::data *px = null;
        if (clr) {
            size_t area = sz.area();
            px = (rgba::data *)malloc(area * sizeof(rgba::data));
            for (int i = 0; i < sz[0] * sz[1]; i++)
                px[i] = clr;
        }
        transfer_pixels(px);
        free(px);
        data.image_ref = false;
    }

    Texture(Device            *device, image                &im,
            VkImageUsageFlags  usage,  VkMemoryPropertyFlags memory,
            VkImageAspectFlags aspect, bool                  ms,
            VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB,
            int                mips   = -1) : Texture({ Data { device, im, usage, memory, aspect, ms, format, mips } }) {
        ///
        rgba::data *px = &im[{0,0}];
        transfer_pixels(px);
        data.image_ref = false;
    }

    Texture(Device            *device, size                  sz,
            VkImage         vk_image,  VkImageView           view,
            VkImageUsageFlags  usage,  VkMemoryPropertyFlags memory,
            VkImageAspectFlags aspect, bool                  ms,
            VkFormat           format = VK_FORMAT_R8G8B8A8_SRGB, int mips = -1) :
        Texture(Data { device, sz, vk_image, view, usage, memory, aspect, ms, format, mips }) { }
    
    void set_stage(Stage s)            { return data.set_stage(s);  }
    void push_stage(Stage s)           { return data.push_stage(s); }
    void pop_stage()                   { return data.pop_stage();   }
    void transfer_pixels(rgba::data *pixels);
    
public:
    /// pass-through operators
    operator  bool                  () { return  data; }
    bool operator!                  () { return !data; }
    bool operator==      (Texture &tx) { return  &data == &tx.data; }
    operator VkImage               &() { return  data; }
    operator VkImageView           &() { return  data; }
    operator VkDeviceMemory        &() { return  data; }
    operator VkAttachmentReference  () { return  data; }
    operator VkAttachmentDescription() { return  data; }
    operator VkDescriptorImageInfo &() { return  data; }
    
    /// pass-through methods
    void destroy() { if (data) data.destroy(); };
    VkWriteDescriptorSet descriptor(VkDescriptorSet &ds, uint32_t binding) {
        return data.descriptor(ds, binding);
    }
};

struct Internal;
/// this is the final final
struct vk_interface {
    Internal *i;
    ///
    vk_interface();
    ///
    VkInstance    &instance();
    uint32_t       version ();
    Texture        texture(size sz);
};

struct ColorTexture:Texture {
    ColorTexture(Device *device, size sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, true, VK_FORMAT_B8G8R8A8_UNORM, 1) { // was VK_FORMAT_B8G8R8A8_SRGB
            data.image_ref = false; /// 'SwapImage' should have this set as its a View-only.. [/creeps away slowly, very unsure of themselves]
            data.push_stage(Stage::Color);
        }
};

/*
return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
);

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
*/

struct DepthTexture:Texture {
    DepthTexture(Device *device, size sz, rgba clr):
        Texture(device, sz, clr,
             VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_DEPTH_BIT, true, VK_FORMAT_D32_SFLOAT, 1) {
            data.image_ref = false; /// we are explicit here.. just plain explicit.
            data.push_stage(Stage::Depth);
        }
};

struct SwapImage:Texture {
    SwapImage(Device *device, size sz, VkImage vk_image):
        Texture(device, sz, vk_image, null,
             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_B8G8R8A8_UNORM, 1) { // VK_FORMAT_B8G8R8A8_UNORM tried here, no effect on SwapImage
            data.layout_override = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
};

const int MAX_FRAMES_IN_FLIGHT = 2;

struct PipelineData;
struct Texture;

struct Render {
    Device            *device;
    std::queue<PipelineData *> sequence; /// sequences of identical pipeline data is purposeful for multiple render passes
    array<VkSemaphore> image_avail;        /// you are going to want ubo controller to update a lambda right before it transfers
    array<VkSemaphore> render_finish;      /// if we're lucky, that could adapt to compute model integration as well.
    array<VkFence>     fence_active;
    array<VkFence>     image_active;
    int                cframe = 0;
    int                sync = 0;
    
    inline void push(PipelineData &pd) {
        sequence.push(&pd);
    }
    ///
    Render(Device * = null);
    void present();
    void update();
};

struct GPU {
protected:
    mx gfx;
    mx present;
public:
    enum Capability {
        Present,
        Graphics,
        Complete
    };
    struct Support {
        VkSurfaceCapabilitiesKHR caps;
        array<VkSurfaceFormatKHR>  formats;
        array<VkPresentModeKHR>    present_modes;
        bool                     ansio;
        VkSampleCountFlagBits    max_sampling;
    };
    Support          support;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    array<VkQueueFamilyProperties> family_props;
    VkSurfaceKHR     surface;
    ///
    GPU(VkPhysicalDevice, VkSurfaceKHR);
    GPU(std::nullptr_t n = null) { }
    uint32_t                index(Capability);
    void                    destroy();
    operator                bool();
    operator                VkPhysicalDevice();
    bool                    operator()(Capability);
};

VkSampleCountFlagBits max_sampling(VkPhysicalDevice gpu) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(gpu, &props);
    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                props.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }
    return VK_SAMPLE_COUNT_1_BIT;
}

///
struct Frame {
    enum Attachment {
        Color,
        Depth,
        SwapView
    };
    ///
    int                   index;
    Device               *device;
    array<Texture>        attachments;
    VkFramebuffer         framebuffer = VK_NULL_HANDLE;
    map<VkCommandBuffer>  render_commands;
    ///
    void destroy();
    void update();
    operator VkFramebuffer &();
    Frame(Device *device);
    Frame() { }
};

struct Device {
    enum Module {
        Vertex,
        Fragment,
        Compute
    };
    
    struct Pair {
        Texture *texture;
        image   *img;
    };

    void                  create_swapchain();
    void                  create_command_buffers();
    void                  create_render_pass();
    uint32_t              attachment_index(Texture::Data *tx);
    void                  initialize(window::data &wdata);

    map<VkShaderModule> f_modules;
    map<VkShaderModule> v_modules;
    Render                render;
    VkSampleCountFlagBits sampling    = VK_SAMPLE_COUNT_8_BIT;
    VkRenderPass          render_pass = VK_NULL_HANDLE;
    Descriptor            desc;
    GPU                   gpu;
    VkCommandPool         command;
    VkDevice              device;
    VkQueue               queues[GPU::Complete];
    VkSwapchainKHR        swap_chain;
    VkFormat              format;
    VkExtent2D            extent;
    VkViewport            viewport;
    VkRect2D              sc;
    array<Frame>          frames;
    array<VkImage>        swap_images; // force re-render on watch event.
    Texture               tx_color;
    Texture               tx_depth;
    map<Pair>             tx_cache;

    static array<VkPhysicalDevice> gpus();
    
    VkShaderModule module(Path p, Assets &rsc, Module type);
    ///
    /// todo: initialize for compute_only
    void            update();
    void            destroy();
    VkCommandBuffer begin();
    void            submit(VkCommandBuffer commandBuffer);
    uint32_t        memory_type(uint32_t types, VkMemoryPropertyFlags props);
    
    operator        VkPhysicalDevice();
    operator        VkDevice();
    operator        VkCommandPool();
    operator        VkRenderPass();
    Device &sync();
    Device(GPU &gpu, bool aa = false);
    VkQueue &operator()(GPU::Capability cap);
    ///
    Device(std::nullptr_t n = null) { }
    operator VkViewport &() { return viewport; }
    operator VkPipelineViewportStateCreateInfo() {
        sc.offset = { 0, 0 };
        sc.extent = extent;
        return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, null, 0, 1, &viewport, 1, &sc };
    }
    operator VkPipelineRasterizationStateCreateInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, null, 0, VK_FALSE, VK_FALSE,
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE // ... = 0.0f
        };
    }
};

/// create with solid color
Texture::Data::Data(Device *device, size sz, rgba clr,
                 VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                 VkImageAspectFlags a, bool ms,
                 VkFormat format, int mips, image lazy) :
                    device(device), sz(sz),
                    mips(auto_mips(mips, sz)), mprops(m),
                    ms(ms), format(format), usage(u), aflags(a), lazy(lazy) { }

/// create with image (rgba required)
Texture::Data::Data(Device *device, image &im,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms,
                    VkFormat           f, int  mips) :
                       device(device), sz(size { num(im.height()), num(im.width()) }), mips(auto_mips(mips, sz)),
                       mprops(m),  ms(ms),  format(f),  usage(u), aflags(a),  lazy(im) { }

Texture::Data::Data(Device *device, size sz, VkImage vk_image, VkImageView view,
                    VkImageUsageFlags  u, VkMemoryPropertyFlags m,
                    VkImageAspectFlags a, bool ms, VkFormat format, int mips):
                       device(device),  vk_image(vk_image),              view(view),
                           sz(sz),          mips(auto_mips(mips, sz)), mprops(m),
                           ms(ms),     image_ref(true),                format(format),
                        usage(u),         aflags(a) { }

Texture::Stage::Stage(Stage::Type value) : value(value) { }

const Texture::Stage::Data Texture::Stage::types[5] = {
    { Texture::Stage::Undefined, VK_IMAGE_LAYOUT_UNDEFINED,                0,                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
    { Texture::Stage::Transfer,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
    { Texture::Stage::Shader,    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  },
    { Texture::Stage::Color,     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    { Texture::Stage::Depth,     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }
};

void Texture::Data::create_sampler() {
    Device &device = *this->device;
    VkPhysicalDeviceProperties props { };
    vkGetPhysicalDeviceProperties(device, &props);
    ///
    VkSamplerCreateInfo si {};
    si.sType                     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter                 = VK_FILTER_LINEAR;
    si.minFilter                 = VK_FILTER_LINEAR;
    si.addressModeU              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeV              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.addressModeW              = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.anisotropyEnable          = VK_TRUE;
    si.maxAnisotropy             = props.limits.maxSamplerAnisotropy;
    si.borderColor               = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    si.unnormalizedCoordinates   = VK_FALSE;
    si.compareEnable             = VK_FALSE;
    si.compareOp                 = VK_COMPARE_OP_ALWAYS;
    si.mipmapMode                = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.minLod                    = 0.0f;
    si.maxLod                    = float(mips);
    si.mipLodBias                = 0.0f;
    ///
    VkResult r = vkCreateSampler(device, &si, nullptr, &sampler);
    assert  (r == VK_SUCCESS);
}

void Texture::Data::create_resources() {
    Device &device = *this->device;
    VkImageCreateInfo imi {};
    ///
    layout_override             = VK_IMAGE_LAYOUT_UNDEFINED;
    ///
    imi.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imi.imageType               = VK_IMAGE_TYPE_2D;
    imi.extent.width            = u32(sz[1]);
    imi.extent.height           = u32(sz[0]);
    imi.extent.depth            = 1;
    imi.mipLevels               = mips;
    imi.arrayLayers             = 1;
    imi.format                  = format;
    imi.tiling                  = tiling;
    imi.initialLayout           = layout_override;
    imi.usage                   = usage;
    imi.samples                 = ms ? device.sampling : VK_SAMPLE_COUNT_1_BIT;
    imi.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    ///
    assert(vkCreateImage(device, &imi, nullptr, &vk_image) == VK_SUCCESS);
    ///
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, vk_image, &req);
    VkMemoryAllocateInfo a {};
    a.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    a.allocationSize            = req.size;
    a.memoryTypeIndex           = device.memory_type(req.memoryTypeBits, mprops); /// this should work; they all use the same mprops from what i've seen so far and it stores that default
    assert(vkAllocateMemory(device, &a, nullptr, &memory) == VK_SUCCESS);
    vkBindImageMemory(device, vk_image, memory, 0);
    ///
    if (ms) create_sampler();
    ///
    // run once like this:
    //set_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); /// why?
}

void Texture::Data::pop_stage() {
    assert(stage.size() >= 2);
    /// kind of need to update it.
    stage.pop();
    set_stage(stage.top());
}

void Texture::Data::set_stage(Stage next_stage) {
    Device  & device = *this->device;
    Stage       cur  = prv_stage;
    while (cur != next_stage) {
        Stage  from  = cur;
        Stage  to    = cur;
        ///
        /// skip over that color thing because this is quirky
        do to = Stage::Type(int(to) + ((next_stage > prv_stage) ? 1 : -1));
        while (to == Stage::Color && to != next_stage);
        ///
        /// transition through the image membrane, as if we were insane in-the
        Texture::Stage::Data      data = from.data();
        Texture::Stage::Data next_data =   to.data();
        VkImageMemoryBarrier   barrier = {
            .sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask        = VkAccessFlags(     data.access),
            .dstAccessMask        = VkAccessFlags(next_data.access),
            .oldLayout            =      data.layout,
            .newLayout            = next_data.layout,
            .srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED,
            .image                = vk_image,
            .subresourceRange     = {
                .aspectMask       = aflags,
                .baseMipLevel     = 0,
                .levelCount       = uint32_t(mips),
                .baseArrayLayer   = 0,
                .layerCount       = 1
            }
        };
        VkCommandBuffer cb = device.begin();
        vkCmdPipelineBarrier(
            cb, data.stage, next_data.stage, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);
        device.submit(cb);
        cur = to;
    }
    prv_stage = next_stage;
}

void Texture::Data::push_stage(Stage next_stage) {
    if (stage.size() == 0)
        stage.push(Stage::Undefined); /// default stage, when pop'd
    if (next_stage == stage.top())
        return;
    
    set_stage(next_stage);
    stage.push(next_stage);
}

void Texture::transfer_pixels(rgba::data *pixels) { /// sz = 0, eh.
    Data   &d      = data;
    Device &device = *d.device;
    ///
    d.create_resources(); // usage not copied?
    if (pixels) {
        auto    nbytes = VkDeviceSize(d.sz.area() * 4); /// adjust bytes if format isnt rgba; implement grayscale
        Buffer staging = Buffer(&device, nbytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        ///
        void* mem = null;
        vkMapMemory(device, staging, 0, nbytes, 0, &mem);
        memcpy(mem, pixels, size_t(nbytes));
        vkUnmapMemory(device, staging);
        ///
        push_stage(Stage::Transfer);
        staging.copy_to(this);
        //pop_stage(); // ah ha!..
        staging.destroy();
    }
    ///if (mip > 0) # skipping this for now
    ///    generate_mipmaps(device, image, format, sz.data.x, sz.data.y, mip);
}

VkImageView Texture::Data::create_view(VkDevice device, size &sz, VkImage vk_image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageView   view;
    uint32_t      mips = auto_mips(mip_levels, sz);
  //auto         usage = VkImageViewUsageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, null, VK_IMAGE_USAGE_SAMPLED_BIT };
    auto             v = VkImageViewCreateInfo {};
    v.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  //v.pNext            = &usage;
    v.image            = vk_image;
    v.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    v.format           = format;
    v.subresourceRange = { aspect_flags, 0, mips, 0, 1 };
    VkResult res = vkCreateImageView(device, &v, nullptr, &view);
    assert (res == VK_SUCCESS);
    return view;
}

int Texture::Data::auto_mips(uint32_t mips, size &sz) {
    return 1; /// mips == 0 ? (uint32_t(std::floor(std::log2(sz.max()))) + 1) : std::max(1, mips);
}

Texture::Data::operator bool () { return vk_image != VK_NULL_HANDLE; }
bool Texture::Data::operator!() { return vk_image == VK_NULL_HANDLE; }

Texture::Data::operator VkImage &() {
    return vk_image;
}

Texture::Data::operator VkImageView &() {
    if (!view)
         view = create_view(device_handle(device), sz, vk_image, format, aflags, mips); /// hide away the views, only create when they are used
    return view;
}

Texture::Data::operator VkDeviceMemory &() {
    return memory;
}

Texture::Data::operator VkAttachmentDescription() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    bool is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto     desc = VkAttachmentDescription {
        .format         = format,
        .samples        = ms ? device->sampling : VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = image_ref ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                      VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        =  is_color ? VK_ATTACHMENT_STORE_OP_STORE :
                                      VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        /// odd thing to need to pass the life cycle like this
        .finalLayout    = image_ref  ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                           is_color  ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    return desc;
}

Texture::Data::operator VkAttachmentReference() {
    assert(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
           usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  //bool  is_color = usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    uint32_t index = device->attachment_index(this);
    return VkAttachmentReference {
        .attachment  = index,
        .layout      = (layout_override != VK_IMAGE_LAYOUT_UNDEFINED) ? layout_override :
                       (stage.size() ? stage.top().data().layout : VK_IMAGE_LAYOUT_UNDEFINED)
    };
}

VkImageView Texture::Data::image_view() {
    if (!view)
         view = create_view(*device, sz, vk_image, format, aflags, mips);
    return view;
}

VkSampler Texture::Data::image_sampler() {
    if (!sampler) create_sampler();
    return sampler;
}

Texture::Data::operator VkDescriptorImageInfo &() {
    info  = {
        .sampler     = image_sampler(),
        .imageView   = image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    return info;
}

///
void Texture::Data::destroy() {
    if (device) {
        vkDestroyImageView(*device, view,     nullptr);
        vkDestroyImage(*device,     vk_image, nullptr);
        vkFreeMemory(*device,       memory,   nullptr);
    }
}

VkWriteDescriptorSet Texture::Data::descriptor(VkDescriptorSet &ds, uint32_t binding) {
    VkDescriptorImageInfo &info = *this;
    return {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = ds,
        .dstBinding      = binding,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo      = &info
    };
}

Buffer::Buffer(Device *device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) :
        device(device), sz(size) {
    VkBufferCreateInfo bi {};
    bi.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size         = VkDeviceSize(size);
    bi.usage        = usage;
    bi.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;
    ///
    console.test(vkCreateBuffer(*device, &bi, nullptr, &buffer) == VK_SUCCESS);
    ///
    /// fetch 'requirements'
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(*device, buffer, &req);
    ///
    /// allocate and bind
    VkMemoryAllocateInfo alloc {};
    alloc.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize    = req.size;
    alloc.memoryTypeIndex   = device->memory_type(req.memoryTypeBits, properties);
    console.test(vkAllocateMemory(*device, &alloc, nullptr, &memory) == VK_SUCCESS);
    vkBindBufferMemory(*device, buffer, memory, 0);
    ///
    info = VkDescriptorBufferInfo {};
    info.buffer = buffer;
    info.offset = 0;
    info.range  = size; /// was:sizeof(UniformBufferObject)
}

/// no crop support yet; simple
void Buffer::copy_to(Texture *tx) {
    auto device = *this->device;
    auto cmd    = device.begin();
    auto reg    = VkBufferImageCopy {};
    auto &td    = tx->data;
    reg.imageSubresource = { td.aflags, 0, 0, 1 };
    reg.imageExtent      = { uint32_t(td.sz[1]), uint32_t(td.sz[0]), 1 };
    vkCmdCopyBufferToImage(cmd, buffer, *tx, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
    device.submit(cmd);
}

void Buffer::copy_to(Buffer &dst, size_t size) {
    auto cb = device->begin();
    VkBufferCopy params {};
    params.size = VkDeviceSize(size);
    vkCmdCopyBuffer(cb, *this, dst, 1, &params);
    device->submit(cb);
}

Buffer::operator VkDeviceMemory() {
    return memory;
}

Buffer::operator VkBuffer() {
    return buffer;
}

void Buffer::destroy() {
    vkDestroyBuffer(*device, buffer, null);
    vkFreeMemory(*device, memory, null);
}

typedef std::function<void(void *)> UniformFn;

struct UniformData {
    struct Memory {
        Device       *device = null;
        int           struct_sz;
        array<Buffer> buffers;
        UniformFn     fn;
    };
    std::shared_ptr<Memory> m = null;
    
    UniformData(std::nullptr_t n = null) { }
    VkWriteDescriptorSet descriptor(size_t frame_index, VkDescriptorSet &ds);
    void   destroy();
    void   update(Device *device);
    void   transfer(size_t frame_id);
    inline operator  bool() { return  m && m->device != null; }
    inline bool operator!() { return !operator bool(); }
    
    /// shouldnt need the serialization
    /*
    UniformData(var &v):
        m(std::shared_ptr<Memory>(new Memory {
            .struct_sz = v["struct_sz"],
            .fn        = v["fn"].convert<UniformFn>()
        })) { }
    ///
    operator var() {
        size_t uni_sz = sizeof(UniformFn);
        std::shared_ptr<uint8_t> b(new uint8_t[uni_sz]);
        memcopy(b.get(), (uint8_t *)&m->fn, uni_sz);
        return Map {
            {"sz", m->struct_sz},
            {"fn", var { Type::ui8, b, uni_sz }}
        };
    }*/
};

template <typename U>
struct UniformBuffer:UniformData { /// will be best to call it 'Uniform', 'Verts', 'Polys'; make sensible.
    UniformBuffer(std::nullptr_t n = null) { }
    UniformBuffer(Device &device, std::function<void(U &)> fn) {
        m = std::shared_ptr<Memory>(new Memory {
            .device    = &device,
            .struct_sz = sizeof(U),
            .fn        = UniformFn([fn](void *u) { fn(*(U *)u); })
        });
    }
};

/// obtain write desciptor set
VkWriteDescriptorSet UniformData::descriptor(size_t frame_index, VkDescriptorSet &ds) {
    auto &m = *this->m;
    return {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, null, ds, 0, 0,
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, null, &m.buffers[frame_index].info, null
    };
}

/// lazy initialization of Uniforms make the component tree work a bit better
void UniformData::update(Device *device) {
    auto &m = *this->m;
    //for (auto &b: m.buffers)
    //    b.destroy();
    m.device = device;
    size_t n_frames = device->frames.len();
    m.buffers = array<Buffer>(n_frames);
    for (int i = 0; i < n_frames; i++)
        m.buffers += Buffer {
            device, uint32_t(m.struct_sz), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
}

void UniformData::destroy() {
    auto &m = *this->m;
    size_t n_frames = m.device->frames.len();
    for (int i = 0; i < n_frames; i++)
        m.buffers[i].destroy();
}

/// called by the Render
void UniformData::transfer(size_t frame_id) {
    auto        &m = *this->m;
    auto   &device = *m.device;
    Buffer &buffer = m.buffers[frame_id]; /// todo -- UniformData never initialized (problem, that is)
    size_t      sz = buffer.sz;
    cchar_t *data[2048];
    void* gpu;
    memset(data, 0, sizeof(data));
    m.fn(data);
    vkMapMemory(device, buffer, 0, sz, 0, &gpu);
    memcpy(gpu, data, sz);
    vkUnmapMemory(device, buffer);
}

VkDevice device_handle(Device *device) {
    return device->device;
}

VkPhysicalDevice gpu_handle(Device *device) {
    return device->gpu;
}

uint32_t memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props) {
   VkPhysicalDeviceMemoryProperties mprops;
   vkGetPhysicalDeviceMemoryProperties(gpu, &mprops);
   ///
   for (uint32_t i = 0; i < mprops.memoryTypeCount; i++)
       if ((types & (1 << i)) && (mprops.memoryTypes[i].propertyFlags & props) == props)
           return i;
   /// no flags match
   assert(false);
   return 0;
};

/// no question we would have to pass in teh arguments used for this shader as well
/// that is the texture resource combo used
VkShaderModule Device::module(Path path, Assets &assets, Module type) {
    str    key = path;
    auto    &m = type == Vertex ? v_modules : f_modules;
    Path   spv = fmt {"{0}.spv", { key }};
    
#if !defined(NDEBUG)
    if (!spv.exists() || path.modified_at() > spv.modified_at()) {
        if (m.count(key))
            m.remove(key);
        str   defs = "";
        auto remap = map<str> {
            { Asset::Color,    " -DCOLOR"    },
            { Asset::Specular, " -DSPECULAR" },
            { Asset::Displace, " -DDISPLACE" },
            { Asset::Normal,   " -DNORMAL"   }
        };
        
        for (auto &[type, tx]: assets) /// always use type when you can, over id (id is genuinely instanced things)
            defs += remap[type];
        
        /// path was /usr/local/bin/ ; it should just be in path, and to support native windows
        str     command = fmt   {"glslc {1} {0} -o {0}.spv", { key, defs }};
        async { command }.sync();
        
        ///
        Path tmp = "./.tmp/"; ///
        if (spv.exists() && spv.modified_at() >= path.modified_at()) {
            spv.copy(tmp);
        } else {
            // look for tmp. if this does not exist we can not proceed.
            // compilation failure, so look for temp which worked before.
        }
        
        /// if it succeeds, the spv is written and that will have a greater modified-at
    }
#endif

    if (!m.count(key)) {
        auto mc     = VkShaderModuleCreateInfo { };
        str code    = str::read_file(spv);
        mc.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        mc.codeSize = code.len();
        mc.pCode    = reinterpret_cast<const uint32_t *>(code.cs());
        assert (vkCreateShaderModule(device, &mc, null, &m[key]) == VK_SUCCESS);
    }
    return m[key];
}

uint32_t Device::memory_type(uint32_t types, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mprops;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mprops);
    ///
    for (uint32_t i = 0; i < mprops.memoryTypeCount; i++)
        if ((types & (1 << i)) && (mprops.memoryTypes[i].propertyFlags & props) == props)
            return i;
    /// no flags match
    assert(false);
    return 0;
};

Device &Device::sync() {
    usleep(1000); // sync the fencing things.
    return *this;
}

Device::Device(GPU &p_gpu, bool aa) {
    auto qcreate        = array<VkDeviceQueueCreateInfo>(2);
    gpu                 = GPU(p_gpu);
    sampling            = VK_SAMPLE_COUNT_8_BIT; //aa ? gpu.support.max_sampling : VK_SAMPLE_COUNT_1_BIT;
    float priority      = 1.0f;
    uint32_t i_gfx      = gpu.index(GPU::Graphics);
    uint32_t i_present  = gpu.index(GPU::Present);
    qcreate            += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_gfx,     1, &priority };
    if (i_present != i_gfx)
        qcreate        += VkDeviceQueueCreateInfo  { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, null, 0, i_present, 1, &priority };
    auto features       = VkPhysicalDeviceFeatures { .samplerAnisotropy = aa ? VK_TRUE : VK_FALSE };
    bool is_apple = false;
    ///
    /// silver: macros should be better, not removed. wizard once told me.
#ifdef __APPLE__
    is_apple = true;
#endif
    array<cchar_t *> ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkDeviceCreateInfo ci {};
#if !defined(NDEBUG)
    //ext += VK_EXT_DEBUG_UTILS_EXTENSION_NAME; # not supported on macOS with lunarg vk
    static cchar_t *debug   = "VK_LAYER_KHRONOS_validation";
    ci.enabledLayerCount       = 1;
    ci.ppEnabledLayerNames     = &debug;
#endif
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = uint32_t(qcreate.len());
    ci.pQueueCreateInfos       = qcreate.data();
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = uint32_t(ext.len() - size_t(!is_apple));
    ci.ppEnabledExtensionNames = ext.data();

    auto res = vkCreateDevice(gpu, &ci, nullptr, &device);
    assert(res == VK_SUCCESS);
    vkGetDeviceQueue(device, gpu.index(GPU::Graphics), 0, &queues[GPU::Graphics]); /// switch between like-queues if this is a hinderance
    vkGetDeviceQueue(device, gpu.index(GPU::Present),  0, &queues[GPU::Present]);
    /// create command pool (i think the pooling should be on each Frame)
    auto pool_info = VkCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = gpu.index(GPU::Graphics)
    };
    assert(vkCreateCommandPool(device, &pool_info, nullptr, &command) == VK_SUCCESS);
}

/// this is to avoid doing explicit monkey-work, and keep code size down as well as find misbound texture
uint32_t Device::attachment_index(Texture::Data *tx) {
    auto is_referencing = [&](Texture::Data *a_data, Texture *b) {
        if (!a_data && !b->data)
            return false;
        if (a_data && b->data && a_data->vk_image && a_data->vk_image == b->data.vk_image)
            return true;
        if (a_data && b->data && a_data->view  && a_data->view  == b->data.view)
            return true;
        return false;
    };
    for (auto &f: frames)
        for (int i = 0; i < f.attachments.len(); i++)
            if (is_referencing(tx, &f.attachments[i]))
                return i;
    assert(false);
    return 0;
}

/// this is an initialize as well, and its also called when things change (resize or pipeline changes)
/// recreate pipeline may be a better name
void Device::update() {
    auto sz          = size { num(extent.height), num(extent.width) };
    u32  frame_count = u32(frames.len());
    render.sync++;
    tx_color = ColorTexture { this, sz, null };
    tx_depth = DepthTexture { this, sz, null };
    /// should effectively perform: ( Undefined -> Transfer, Transfer -> Shader, Shader -> Color )
    for (size_t i = 0; i < frame_count; i++) {
        Frame &frame   = frames[i];
        frame.index    = int(i);
        auto   tx_swap = SwapImage { this, sz, swap_images[i] };
        ///
        frame.attachments = array<Texture> {
            tx_color,
            tx_depth,
            tx_swap
        };
    }
    ///
    create_render_pass();
    tx_color.push_stage(Texture::Stage::Shader);
    tx_depth.push_stage(Texture::Stage::Shader);
    ///
    for (size_t i = 0; i < frame_count; i++) {
        Frame &frame = frames[i];
        frame.update();
    }
    tx_color.pop_stage();
    tx_depth.pop_stage();
    /// render needs to update
    /// render.update();
}

void Device::initialize(window::data &wdata) {
    auto select_surface_format = [](array<VkSurfaceFormatKHR> &formats) -> VkSurfaceFormatKHR {
        for (const auto &f: formats)
            if (f.format     == VK_FORMAT_B8G8R8A8_UNORM && // VK_FORMAT_B8G8R8A8_UNORM in an attempt to fix color issues, no effect.
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // the only colorSpace available in list of formats
                return f;
        return formats[0];
    };
    ///
    auto select_present_mode = [](array<VkPresentModeKHR> &modes) -> VkPresentModeKHR {
        for (const auto &m: modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    };
    ///
    auto swap_extent = [pw=&wdata](VkSurfaceCapabilitiesKHR& caps) -> VkExtent2D {
        if (caps.currentExtent.width != UINT32_MAX) {
            return caps.currentExtent;
        } else {
            int w, h;
            glfwGetFramebufferSize(pw->glfw, &w, &h);
            VkExtent2D ext = { uint32_t(w), uint32_t(h) };
            ext.width      = std::clamp(ext.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
            ext.height     = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            return ext;
        }
    };
    ///
    auto     support              = gpu.support;
    uint32_t gfx_index            = gpu.index(GPU::Graphics);
    uint32_t present_index        = gpu.index(GPU::Present);
    auto     surface_format       = select_surface_format(support.formats);
    auto     surface_present      = select_present_mode(support.present_modes);
    auto     extent               = swap_extent(support.caps);
    uint32_t frame_count          = support.caps.minImageCount + 1;
    uint32_t indices[]            = { gfx_index, present_index };
    ///
    if (support.caps.maxImageCount > 0 && frame_count > support.caps.maxImageCount)
        frame_count               = support.caps.maxImageCount;
    ///
    VkSwapchainCreateInfoKHR ci {};
    ci.sType                      = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface                    = wdata.vk_surface;
    ci.minImageCount              = frame_count;
    ci.imageFormat                = surface_format.format;
    ci.imageColorSpace            = surface_format.colorSpace;
    ci.imageExtent                = extent;
    ci.imageArrayLayers           = 1;
    ci.imageUsage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ///
    if (gfx_index != present_index) {
        ci.imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount  = 2;
        ci.pQueueFamilyIndices    = indices;
    } else
        ci.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    ///
    ci.preTransform               = support.caps.currentTransform;
    ci.compositeAlpha             = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode                = surface_present;
    ci.clipped                    = VK_TRUE;
    
    /// create swap-chain
    assert(vkCreateSwapchainKHR(device, &ci, nullptr, &swap_chain) == VK_SUCCESS);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, nullptr); /// the dreaded frame_count (2) vs image_count (3) is real.
    frames = array<Frame>(size_t(frame_count), Frame(this));
    
    /// get swap-chain images
    swap_images.set_size(frame_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &frame_count, swap_images.data());
    format             = surface_format.format;
    this->extent       = extent;
    viewport           = { 0.0f, 0.0f, r32(extent.width), r32(extent.height), 0.0f, 1.0f };
    const int guess    = 8;
    auto      ps       = std::array<VkDescriptorPoolSize, 3> {};
    ps[0]              = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         frame_count * guess };
    ps[1]              = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count * guess };
    ps[2]              = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frame_count * guess };
    auto dpi           = VkDescriptorPoolCreateInfo {
                            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, null, 0,
                            frame_count * guess, uint32_t(frame_count), ps.data() };
    render             = Render(this);
    desc               = Descriptor(this);
    ///
    assert(vkCreateDescriptorPool(device, &dpi, nullptr, &desc.pool) == VK_SUCCESS);
    for (auto &f: frames)
        f.index        = -1;
    update();
}

void Descriptor::destroy() {
    auto &device = *this->device;
    vkDestroyDescriptorPool(device, pool, nullptr);
}

#if 0
VkFormat supported_format(VkPhysicalDevice gpu, array<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &props);

        bool fmatch = (props.linearTilingFeatures & features) == features;
        bool tmatch = (tiling == VK_IMAGE_TILING_LINEAR || tiling == VK_IMAGE_TILING_OPTIMAL);
        if  (fmatch && tmatch)
            return format;
    }

    throw std::runtime_error("failed to find supported format!");
}

static VkFormat get_depth_format(VkPhysicalDevice gpu) {
    return supported_format(gpu,
        {VK_FORMAT_D32_SFLOAT,    VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
         VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
};
#endif



void Device::create_render_pass() {
    Frame &f0                     = frames[0];
    Texture &tx_ref               = f0.attachments[Frame::SwapView]; /// has msaa set to 1bit
    assert(f0.attachments.len() == 3);
    VkAttachmentReference    cref = tx_color; // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    dref = tx_depth; // VK_IMAGE_LAYOUT_UNDEFINED
    VkAttachmentReference    rref = tx_ref;   // the odd thing is the associated COLOR_ATTACHMENT layout here;
    VkSubpassDescription     sp   = { 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, null, 1, &cref, &rref, &dref };

    VkSubpassDependency dep { };
    dep.srcSubpass                = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass                = 0;
    dep.srcStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask             = 0;
    dep.dstStageMask              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask             = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription adesc_clr_image  = f0.attachments[0];
    VkAttachmentDescription adesc_dep_image  = f0.attachments[1]; // format is wrong here, it should be the format of the tx
    VkAttachmentDescription adesc_swap_image = f0.attachments[2];
    
    std::array<VkAttachmentDescription, 3> attachments = {adesc_clr_image, adesc_dep_image, adesc_swap_image};
    VkRenderPassCreateInfo rpi { }; // VkAttachmentDescription needs to be 4, 4, 1
    rpi.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount           = uint32_t(attachments.size());
    rpi.pAttachments              = attachments.data();
    rpi.subpassCount              = 1;
    rpi.pSubpasses                = &sp;
    rpi.dependencyCount           = 1;
    rpi.pDependencies             = &dep;
    ///
    /// seems like tx_depth is not created with a 'depth' format, or improperly being associated to a color format
    ///
    assert(vkCreateRenderPass(device, &rpi, nullptr, &render_pass) == VK_SUCCESS);
}

VkCommandBuffer Device::begin() {
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo ai {};
    ///
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool        = command;
    ai.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &ai, &cb);
    ///
    VkCommandBufferBeginInfo bi {};
    bi.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    return cb;
}

void Device::submit(VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);
    ///
    VkSubmitInfo si {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cb;
    ///
    VkQueue &queue          = queues[GPU::Graphics];
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    ///
    vkFreeCommandBuffers(device, command, 1, &cb);
}

VkQueue &Device::operator()(GPU::Capability cap) {
    assert(cap != GPU::Complete);
    return queues[cap];
}

Device::operator VkPhysicalDevice() {
    return gpu;
}

Device::operator VkCommandPool() {
    return command;
}

Device::operator VkDevice() {
    return device;
}

Device::operator VkRenderPass() {
    return render_pass;
}

///
void Device::destroy() {
    for (auto &f: frames)
        f.destroy();
    for (field<VkShaderModule> &field: f_modules)
        vkDestroyShaderModule(device, field.value, null);
    for (field<VkShaderModule> &field: v_modules)
        vkDestroyShaderModule(device, field.value, null);
    vkDestroyRenderPass(device, render_pass, null);
    vkDestroySwapchainKHR(device, swap_chain, null);
    desc.destroy();
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

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

Frame::Frame(Device *device): device(device) { }

/// axe these as opaque.
template <> struct is_opaque<VkImageView>       : true_type { };
template <> struct is_opaque<VkPhysicalDevice>  : true_type { };
template <> struct is_opaque<VkImage>           : true_type { };
template <> struct is_opaque<VkFence>           : true_type { };
template <> struct is_opaque<VkSemaphore>       : true_type { };
template <> struct is_opaque<VkBuffer>          : true_type { };
template <> struct is_opaque<VkCommandBuffer>   : true_type { };
template <> struct is_opaque<VkDescriptorSet>   : true_type { };
template <> struct is_opaque<VkLayerProperties> : true_type { };

///
void Frame::update() {
    Device &device = *this->device;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    auto views = array<VkImageView>();
    for (Texture &tx: attachments)
        views += VkImageView(tx);
    VkFramebufferCreateInfo ci {};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = device.render_pass;
    ci.attachmentCount = u32(views.len());
    ci.pAttachments    = views.data();
    ci.width           = device.extent.width;
    ci.height          = device.extent.height;
    ci.layers          = 1;
    assert(vkCreateFramebuffer(device, &ci, nullptr, &framebuffer) == VK_SUCCESS);
}

///
void Frame::destroy() {
    auto &device = *this->device;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}

Frame::operator VkFramebuffer &() {
    return framebuffer;
}

/// surface handle passed in for all gpus, its going to be the same one for each
GPU::GPU(VkPhysicalDevice gpu, VkSurfaceKHR surface) : gpu(gpu) {
    uint32_t fcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, nullptr);
    family_props.set_size(fcount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &fcount, family_props.data());
    uint32_t index = 0;
    /// we need to know the indices of these
    gfx     = mx(false);
    present = mx(false);
    
    for (const auto &fprop: family_props) { // check to see if it steps through data here.. thats the question
        VkBool32 has_present;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &has_present);
        bool has_gfx = (fprop.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        ///
        /// prefer to get gfx and present in the same family_props
        if (has_gfx) {
            gfx = mx(index);
            /// query surface formats
            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
            if (format_count != 0) {
                support.formats.set_size(format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                    gpu, surface, &format_count, support.formats.data());
            }
            /// query swap chain support, store on GPU
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                gpu, surface, &support.caps);
            /// ansio relates to gfx
            VkPhysicalDeviceFeatures fx;
            vkGetPhysicalDeviceFeatures(gpu, &fx);
            support.ansio = fx.samplerAnisotropy;
        }
        ///
        if (bool(has_present)) {
            present = var(index);
            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                gpu, surface, &present_mode_count, nullptr);
            if (present_mode_count != 0) {
                support.present_modes.set_size(present_mode_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                    gpu, surface, &present_mode_count, support.present_modes.data());
            }
        }
        if (bool(has_gfx) & bool(has_present))
            break;
        index++;
    }
    if (support.ansio)
        support.max_sampling = max_sampling(gpu);
}

/// name better, ...this.
struct vkState {
    VkPipelineVertexInputStateCreateInfo    vertex_info {};
    VkPipelineInputAssemblyStateCreateInfo  topology    {};
    VkPipelineViewportStateCreateInfo       vs          {};
    VkPipelineRasterizationStateCreateInfo  rs          {};
    VkPipelineMultisampleStateCreateInfo    ms          {};
    VkPipelineDepthStencilStateCreateInfo   ds          {};
    VkPipelineColorBlendAttachmentState     cba         {};
    VkPipelineColorBlendStateCreateInfo     blending    {};
};

typedef std::function<void(vkState &)> VkStateFn;

/// explicit use of the implicit constructor with this comment
struct PipelineData {
    struct Memory {
        Device                    *device;
        str                        shader;
        array<VkCommandBuffer>     frame_commands;   /// pipeline are many and even across swap frame idents, and we need a ubo and cmd set for each
        array<VkDescriptorSet>     desc_sets;        /// needs to be in frame, i think.
        VkPipelineLayout           pipeline_layout;  /// and what a layout
        VkPipeline                 pipeline;         /// pipeline handle
        UniformData                ubo;              /// we must broadcast this buffer across to all of the swap uniforms
        VertexData                 vbo;              ///
        IndexData                  ibo;
        // array<VkVertexInputAttributeDescription> attr; -- vector is given via VertexData::fn_attribs(). i dont believe those need to be pushed up the call chain
        Assets                     assets;
        VkDescriptorSetLayout      set_layout;
        bool                       enabled = true;
        map<Texture *>             tx_cache;
        size_t                     vsize;
        rgba                       clr;
        ///
        /// state vars
        int                        sync = -1;
        ///
        operator bool ();
        bool operator!();
        void enable (bool en);
        void update(size_t frame_id);
        Memory(std::nullptr_t    n = null);
        Memory(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
               Assets &assets, size_t vsize, rgba clr, str name, VkStateFn vk_state);
        void destroy();
        void initialize();
        ~Memory();
    };
    std::shared_ptr<Memory> m;
    ///
    operator bool  ()                { return  m->enabled; }
    bool operator! ()                { return !m->enabled; }
    void   enable  (bool en)         { m->enabled = en; }
    bool operator==(PipelineData &b) { return m == b.m; }
    PipelineData   (std::nullptr_t n = null) { }
    void   update  (size_t frame_id);
    ///
    PipelineData(Device &device, UniformData &ubo, VertexData &vbo, IndexData &ibo,
                 Assets &assets, size_t vsize, rgba clr, str shader, VkStateFn vk_state = null) {
            m = std::shared_ptr<Memory>(new Memory { device, ubo, vbo, ibo, assets, vsize, clr, shader, vk_state });
        }
};

/// pipeline dx
template <typename V>
struct Pipeline:PipelineData {
    Pipeline(Device &device, UniformData &ubo,  VertexData &vbo,
             IndexData &ibo,   Assets &assets,  rgba clr, str name):
        PipelineData(device, ubo, vbo, ibo, assets, sizeof(V), clr, name) { }
};

/// these are calling for Daniel
struct Pipes:mx {
    struct data {
        Device       *device  = null;
        VertexData    vbo     = null;
        uint32_t      binding = 0;
      //array<Attrib> attr    = {};
        map<PipelineData> part;
    } &d;
    ctr(Pipes, mx, data, d);
    operator bool() { return d.part.count() > 0; }
    inline map<PipelineData> &map() { return d.part; }
};

/// soon: graph-mode shaders? not exactly worth the trouble at the moment especially with live updates not being as quick as glsl recompile
struct Vertex {
    ///
    v3f pos;  // position
    v3f norm; // normal position
    v2f uv;   // texture uv coordinate
    v4f clr;  // color
    v3f ta;   // tangent
    v3f bt;   // bi-tangent, we need an arb in here, or two. or ten.
    
    /// VkAttribs (array of VkAttribute) [data] from VAttribs (state flags) [model]
    static VkAttribs attribs(VAttribs attr) {
        auto res = VkAttribs(6);
        if (attr[VA::Position])  res += { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  pos)  };
        if (attr[VA::Normal])    res += { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  norm) };
        if (attr[VA::UV])        res += { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex,  uv)   };
        if (attr[VA::Color])     res += { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex,  clr)  };
        if (attr[VA::Tangent])   res += { 4, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  ta)   };
        if (attr[VA::BiTangent]) res += { 5, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex,  bt)   };
        return res;
    }
    
    Vertex() { }
    ///
    Vertex(vec3f pos, vec3f norm, vec2f uv, vec4f clr, vec3f ta = {}, vec3f bt = {}):
           pos  (pos.data), norm (norm.data), uv   (uv.data),
           clr  (clr.data), ta   (ta.data),   bt   (bt.data) { }
    
    ///
    Vertex(vec3f &pos, vec3f &norm, vec2f &uv, vec4f &clr, vec3f &ta, vec3f &bt):
           pos  (pos.data), norm (norm.data), uv   (uv.data),
           clr  (clr.data), ta   (ta.data),   bt   (bt.data) { }
    
    ///
    static array<Vertex> square(vec4f v_clr = {1.0, 1.0, 1.0, 1.0}) {
        return array<Vertex> {
            Vertex {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, v_clr},
            Vertex {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, v_clr},
            Vertex {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, v_clr},
            Vertex {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, v_clr}
        };
    }
};

///
template <typename V>
struct VertexBuffer:VertexData {
    VkWriteDescriptorSet operator()(VkDescriptorSet &ds) {
        return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, ds, 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, buffer };
    }
    VertexBuffer(std::nullptr_t n = null) : VertexData(n) { }
    VertexBuffer(Device &device, array<V> &v, VAttribs &attr) : VertexData(device, Buffer {
            &device, v, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
            [attr=attr]() { return V::attribs(attr); }) { }
    size_t size() { return buffer.sz / sizeof(V); }
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


void PipelineData::Memory::destroy() {
    auto  &device = *this->device;
    vkDestroyDescriptorSetLayout(device, set_layout, null);
    vkDestroyPipeline(device, pipeline, null);
    vkDestroyPipelineLayout(device, pipeline_layout, null);
    for (auto &cmd: frame_commands)
        vkFreeCommandBuffers(device, device.command, 1, &cmd);
}
///
PipelineData::Memory::Memory(std::nullptr_t n) { }

PipelineData::Memory::~Memory() {
    destroy();
}

/// constructor for pipeline memory; worth saying again.
PipelineData::Memory::Memory(Device        &device,  UniformData &ubo,
                             VertexData    &vbo,     IndexData   &ibo,
                             Assets     &assets,     size_t       vsize, // replace all instances of array<Texture *> with a map<str, Teture *>
                             rgba           clr,     str       shader,
                             VkStateFn      vk_state):
        device(&device), shader(shader),   ubo(ubo),
           vbo(vbo),        ibo(ibo),   assets(assets),  vsize(vsize),   clr(clr) {
    ///
    auto bindings  = array<VkDescriptorSetLayoutBinding>(1 + assets.count());
         bindings += { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr };
    
    
    /// binding ids for resources are reflected in shader
    /// todo: write big test for this
    for (field<Texture*> &f:assets) {
        Asset  a  = f.key.type() == typeof(int) ? int(f.key) : f.key;
        bindings += Asset_descriptor(a); // likely needs tx in arg form in this generic
    }

    /// create descriptor set layout
    auto desc_layout_info = VkDescriptorSetLayoutCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr, 0, uint32_t(bindings.len()), bindings.data() }; /// todo: fix general pipline. it will be different for Compute Pipeline, but this is reasonable for now if a bit lazy
    VkResult vk_desc_res = vkCreateDescriptorSetLayout(device, &desc_layout_info, null, &set_layout);
    console.test(vk_desc_res == VK_SUCCESS, "vkCreateDescriptorSetLayout failure with {0}", { vk_desc_res });
    
    /// of course, you do something else right about here...
    const size_t  n_frames = device.frames.len();
    auto           layouts = std::vector<VkDescriptorSetLayout>(n_frames, set_layout); /// add to vec.
    auto                ai = VkDescriptorSetAllocateInfo {};
    ai.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool      = device.desc.pool;
    ai.descriptorSetCount  = u32(n_frames);
    ai.pSetLayouts         = layouts.data();
    desc_sets.set_size(n_frames);
    VkDescriptorSet *ds_data = desc_sets.data();
    VkResult res = vkAllocateDescriptorSets(device, &ai, ds_data);
    assert  (res == VK_SUCCESS);
    ubo.update(&device);
    ///
    /// write descriptor sets for all swap image instances
    for (size_t i = 0; i < device.frames.len(); i++) {
        auto &desc_set  = desc_sets[i];
        auto  v_writes  = array<VkWriteDescriptorSet>(size_t(1 + assets.count()));
        
        /// update descriptor sets
        v_writes       += ubo.descriptor(i, desc_set);
        for (field<Texture*> &f:assets) {
            Asset    a  = f.key;
            Texture *tx = f.value;
            v_writes   += tx->descriptor(desc_set, uint32_t(int(a)));
        }
        
        VkDevice    dev = device;
        size_t       sz = v_writes.len();
        VkWriteDescriptorSet *ptr = v_writes.data();
        vkUpdateDescriptorSets(dev, uint32_t(sz), ptr, 0, nullptr);
    }
    str    cwd = Path::cwd();
    str s_vert = str::format("{0}/shaders/{1}.vert", { cwd, shader });
    str s_frag = str::format("{0}/shaders/{1}.frag", { cwd, shader });
    auto  vert = device.module(s_vert, assets, Device::Vertex);
    auto  frag = device.module(s_frag, assets, Device::Fragment);
    ///
    array<VkPipelineShaderStageCreateInfo> stages {{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_VERTEX_BIT,   vert, shader.cs(), null
    }, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, null, 0,
        VK_SHADER_STAGE_FRAGMENT_BIT, frag, shader.cs(), null
    }};
    ///
    auto binding            = VkVertexInputBindingDescription { 0, uint32_t(vsize), VK_VERTEX_INPUT_RATE_VERTEX };
    
    /// 
    auto vk_attr            = vbo.fn_attribs();
    auto viewport           = VkViewport(device);
    auto sc                 = VkRect2D {
        .offset             = {0, 0},
        .extent             = device.extent
    };
    auto cba                = VkPipelineColorBlendAttachmentState {
        .blendEnable        = VK_FALSE,
        .colorWriteMask     = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    auto layout_info        = VkPipelineLayoutCreateInfo {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount     = 1,
        .pSetLayouts        = &set_layout
    };
    ///
    assert(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout) == VK_SUCCESS);
    
    /// ok initializers here we go.
    vkState state {
        .vertex_info = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &binding,
            .vertexAttributeDescriptionCount = uint32_t(vk_attr.len()),
            .pVertexAttributeDescriptions    = vk_attr.data()
        },
        .topology                    = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable  = VK_FALSE
        },
        .vs                          = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount           = 1,
            .pViewports              = &viewport,
            .scissorCount            = 1,
            .pScissors               = &sc
        },
        .rs                          = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_FRONT_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .lineWidth               = 1.0f
        },
        .ms                          = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples    = device.sampling,
            .sampleShadingEnable     = VK_FALSE
        },
        .ds                          = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable         = VK_TRUE,
            .depthWriteEnable        = VK_TRUE,
            .depthCompareOp          = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable   = VK_FALSE,
            .stencilTestEnable       = VK_FALSE
        },
        .blending                    = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable           = VK_FALSE,
            .logicOp                 = VK_LOGIC_OP_COPY,
            .attachmentCount         = 1,
            .pAttachments            = &cba,
            .blendConstants          = { 0.0f, 0.0f, 0.0f, 0.0f }
        }
    };
    ///
    VkGraphicsPipelineCreateInfo pi {
        .sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount                   = uint32_t(stages.len()),
        .pStages                      = stages.data(),
        .pVertexInputState            = &state.vertex_info,
        .pInputAssemblyState          = &state.topology,
        .pViewportState               = &state.vs,
        .pRasterizationState          = &state.rs,
        .pMultisampleState            = &state.ms,
        .pDepthStencilState           = &state.ds,
        .pColorBlendState             = &state.blending,
        .layout                       = pipeline_layout,
        .renderPass                   = device.render_pass,
        .subpass                      = 0,
        .basePipelineHandle           = VK_NULL_HANDLE
    };
    ///
    if (vk_state)
        vk_state(state);
    
    assert (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline) == VK_SUCCESS);
}

/// create graphic pipeline
void PipelineData::Memory::update(size_t frame_id) {
    auto         &device = *this->device;
    Frame         &frame = device.frames[frame_id];
    VkCommandBuffer &cmd = frame_commands[frame_id];
    auto     clear_color = [&](rgba c) {
        return VkClearColorValue {{ float(c->r) / 255.0f, float(c->g) / 255.0f,
                                    float(c->b) / 255.0f, float(c->a) / 255.0f }};
    };
    
    /// reallocate command
    vkFreeCommandBuffers(device, device.command, 1, &cmd);
    auto alloc_info = VkCommandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        null, device.command, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    assert(vkAllocateCommandBuffers(device, &alloc_info, &cmd) == VK_SUCCESS);
    
    /// begin command
    auto begin_info = VkCommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    assert(vkBeginCommandBuffer(cmd, &begin_info) == VK_SUCCESS);
    
    ///
    auto   clear_values = array<VkClearValue> {
        {        .color = clear_color(clr)}, // for image rgba  sfloat
        { .depthStencil = {1.0f, 0}}         // for image depth sfloat
    };
    
    /// nothing in canvas showed with depthStencil = 0.0.
    auto    render_info = VkRenderPassBeginInfo {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = null,
        .renderPass      = VkRenderPass(device),
        .framebuffer     = VkFramebuffer(frame),
        .renderArea      = {{0,0}, device.extent},
        .clearValueCount = u32(clear_values.len()),
        .pClearValues    = clear_values.data()
    };

    /// gather some rendering ingredients
    vkCmdBeginRenderPass(cmd, &render_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    /// toss together a VBO array
    array<VkBuffer>    a_vbo   = { vbo };
    VkDeviceSize   offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, a_vbo.data(), offsets);
    vkCmdBindIndexBuffer(cmd, ibo, 0, ibo.buffer.type_size == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, desc_sets.data(), 0, nullptr);
    
    /// flip around an IBO and toss it in the oven at 410F
    u32 sz_draw = u32(ibo.len());
    vkCmdDrawIndexed(cmd, sz_draw, 1, 0, 0, 0); /// ibo size = number of indices (like a vector)
    vkCmdEndRenderPass(cmd);
    assert(vkEndCommandBuffer(cmd) == VK_SUCCESS);
}


/// the thing that 'executes' is the present.  so this is just 'update'
void Render::update() {
    Device &device = *this->device;
    Frame  &frame  = device.frames[cframe];
    assert (cframe == frame.index);
}

void Render::present() {
    Device &device = *this->device;
    vkWaitForFences(device, 1, &fence_active[cframe], VK_TRUE, UINT64_MAX);
    
    uint32_t image_index; /// look at the ref here
    VkResult result = vkAcquireNextImageKHR(
        device, device.swap_chain, UINT64_MAX,
        image_avail[cframe], VK_NULL_HANDLE, &image_index);
    ///Frame    &frame = device.frames[image_index];
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        assert(false); // lets see when this happens
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        assert(false); // lets see when this happens
    ///
    if (image_active[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &image_active[image_index], VK_TRUE, UINT64_MAX);
    ///
    vkResetFences(device, 1, &fence_active[cframe]);
    VkSemaphore             s_wait[] = {   image_avail[cframe] };
    VkSemaphore           s_signal[] = { render_finish[cframe] };
    VkPipelineStageFlags    f_wait[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    bool sync_diff = false;
    ///
    /// iterate through render sequence, updating out of sync pipelines
    while (sequence.size()) {
        auto &m = *sequence.front()->m;
        sequence.pop();
        
        if (m.sync != sync) {
            /// Device::update() calls change the sync (so initial and updates)
            sync_diff = true;
            m.sync    = sync;
            if (!m.frame_commands)
                 m.frame_commands = array<VkCommandBuffer> (device.frames.len(), VK_NULL_HANDLE);
            for (size_t i = 0; i < device.frames.len(); i++) //
                m.update(i); /// make singular, frame index param
        }
        
        /// transfer uniform struct from lambda call. send us your uniforms!
        m.ubo.transfer(image_index);
        ///
        image_active[image_index]        = fence_active[cframe];
        VkSubmitInfo submit_info         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = s_wait;
        submit_info.pWaitDstStageMask    = f_wait;
        submit_info.pSignalSemaphores    = s_signal;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &m.frame_commands[image_index];
        submit_info.signalSemaphoreCount = 1;
        assert(vkQueueSubmit(device.queues[GPU::Graphics], 1, &submit_info, fence_active[cframe]) == VK_SUCCESS);
        int test = 0;
        test++;
    }
    
    VkPresentInfoKHR     present = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, null, 1,
        s_signal, 1, &device.swap_chain, &image_index };
    VkResult             presult = vkQueuePresentKHR(device.queues[GPU::Present], &present);
    
    if ((presult == VK_ERROR_OUT_OF_DATE_KHR || presult == VK_SUBOPTIMAL_KHR)) {// && !sync_diff) {
        device.update(); /// cframe needs to be valid at its current value
    }// else
     //   assert(result == VK_SUCCESS);
    
    cframe = (cframe + 1) % MAX_FRAMES_IN_FLIGHT; /// ??
}

Render::Render(Device *device): device(device) {
    if (device) {
        const size_t ns  = device->swap_images.len();
        const size_t mx  = MAX_FRAMES_IN_FLIGHT;
        image_avail    = array<VkSemaphore>(mx);
        render_finish  = array<VkSemaphore>(mx);
        fence_active   = array<VkFence>    (mx);
        image_active   = array<VkFence>    (ns);
        ///
        VkSemaphoreCreateInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, null, VK_FENCE_CREATE_SIGNALED_BIT };
        for (size_t i = 0; i < mx; i++)
            assert (vkCreateSemaphore(*device, &si, null,   &image_avail[i]) == VK_SUCCESS &&
                    vkCreateSemaphore(*device, &si, null, &render_finish[i]) == VK_SUCCESS &&
                    vkCreateFence(    *device, &fi, null,  &fence_active[i]) == VK_SUCCESS);
        image_avail  .set_size(mx);
        render_finish.set_size(mx);
        fence_active .set_size(mx);
        image_active .set_size(ns);
    }
}

struct Device;
struct Texture;
///
VkDevice                handle(Device *device);
VkDevice         device_handle(Device *device);
VkPhysicalDevice    gpu_handle(Device *device);
uint32_t           memory_type(VkPhysicalDevice gpu, uint32_t types, VkMemoryPropertyFlags props);

struct Composer;

/// bring in gpu boot strapping from pascal project
array<VkPhysicalDevice> Device::gpus() {
    vk_interface vk;
    VkInstance inst = vk.instance();
    u32 gpu_count;
    
    /// gpu count
    vkEnumeratePhysicalDevices(inst, &gpu_count, null);

    /// allocate arrays for gpus in static form
    static array<VkPhysicalDevice> hw(gpu_count, gpu_count, VK_NULL_HANDLE);
    static bool init;
    if (!init) {
        init = true;
        vkEnumeratePhysicalDevices(inst, &gpu_count, (VkPhysicalDevice*)hw.data());
    }
    return hw;
}

//#include <gpu/gl/GrGLInterface.h>

struct Internal {
    window  *win;
    VkInstance vk = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dmsg;
    array<VkLayerProperties> layers;
    bool     resize;
    array<VkFence> fence;
    array<VkFence> image_fence;
    Internal &bootstrap();
    void destroy();
    static Internal &handle();
};

Internal _;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug(
            VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
            VkDebugUtilsMessageTypeFlagsEXT             type,
            const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
            void*                                       user_data) {
    std::cerr << "validation layer: " << cb_data->pMessage << std::endl;

    return VK_FALSE;
}


Internal &Internal::handle()    { return _.vk ? _ : _.bootstrap(); }

void Internal::destroy() {
    // Destroy the debug messenger and vk instance when finished
    DestroyDebugUtilsMessengerEXT(vk, dmsg, null);
    vkDestroyInstance(vk, null);
}

Internal &Internal::bootstrap() {
    static bool init;
    if (!init) {
        init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    //uint32_t count;
    //vkEnumerateInstanceLayerProperties(&count, nullptr);
    //layers.set_size(count);
    //vkEnumerateInstanceLayerProperties(&count, layers.data());

    const symbol validation_layer   = "VK_LAYER_KHRONOS_validation";
    u32          glfwExtensionCount = 0;
    symbol*      glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    array<symbol> extensions { &glfwExtensions[0], glfwExtensionCount, glfwExtensionCount + 1 };
    extensions += VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    for (symbol sym: extensions) {
        console.log("symbol: {0}", { sym });
    }

    // set the debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debug {
        .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback    = vk_debug
    };

    // set the application info
    VkApplicationInfo app_info   = {
        .sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName    = "ion",
        .applicationVersion  = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName         = "ion",
        .engineVersion       = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion          = VK_MAKE_API_VERSION(0, 1, 0, 0)
    };

    VkInstanceCreateInfo create_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debug,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = 1,
        .ppEnabledLayerNames     = &validation_layer,
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.len()),
        .ppEnabledExtensionNames = extensions.data()
    };

    // create the vk instance
    VkResult vk_result = vkCreateInstance(&create_info, null, &vk);
    console.test(vk_result == VK_SUCCESS, "vk instance failure");

    // create the debug messenger
    VkResult dbg_result = CreateDebugUtilsMessengerEXT(vk, &debug, null, &dmsg);
    console.test(dbg_result == VK_SUCCESS, "vk debug messenger failure");
    return *this;
}

/// cleaning up the vk_instance / internal stuff now
Texture &window::texture() { return *w.tx_skia; }
Texture &window::texture(::size sz) {
    if (w.tx_skia) {
        w.tx_skia->destroy();
        delete w.tx_skia;
    }
    w.tx_skia = new Texture { w.dev, sz, rgba { 1.0, 0.0, 1.0, 1.0 },
        VK_IMAGE_USAGE_SAMPLED_BIT          | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT     | VK_IMAGE_USAGE_TRANSFER_DST_BIT     |
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
        false, VK_FORMAT_B8G8R8A8_UNORM, -1 };
    w.tx_skia->push_stage(Texture::Stage::Color);
    return *w.tx_skia;
}

window::data::~data() {
    if (glfw) {
        printf("destructing data: %p\n", (void*)this);
        int test = 1;
        vk_interface vk;
        vkDestroySurfaceKHR(vk.instance(), vk_surface, null);
        glfwDestroyWindow(glfw);
    }
}

vk_interface::vk_interface() : i(&Internal::handle()) { }
///
//::window      &vk_interface::window  () { return *i->win;        }
//::Device      &vk_interface::device  () { return *i->win->w.dev; }
VkInstance    &vk_interface::instance() { return  i->vk;         }
uint32_t       vk_interface::version () { return VK_MAKE_VERSION(1, 1, 0); }

Device&           window::device()         { return *w.dev; }
VkDevice&         window::vk_device()      { return w.dev->device; }
VkPhysicalDevice& window::gpu()            { return w.dev->gpu.gpu; }
VkQueue&          window::queue()          { return w.dev->queues[GPU::Graphics]; } /// queues should be on gpu unless they are made anew
u32               window::graphics_index() { return w.dev->gpu.index(GPU::Graphics); }

void glfw_key(GLFWwindow *h, int unicode, int scan_code, int action, int mods) {
    window win = window::handle(h);

    /// update modifiers on window
    win.w.modifiers[keyboard::shift] = (mods & GLFW_MOD_SHIFT) != 0;
    win.w.modifiers[keyboard::alt]   = (mods & GLFW_MOD_ALT  ) != 0;
    win.w.modifiers[keyboard::meta]  = (mods & GLFW_MOD_SUPER) != 0;

    win.key_event(
        user::key::data {
            .unicode   = unicode,
            .scan_code = scan_code,
            .modifiers = win.w.modifiers,
            .repeat    = action == GLFW_REPEAT,
            .up        = action == GLFW_RELEASE
        }
    );
}

void glfw_char(GLFWwindow *h, u32 unicode) { window::handle(h).char_event(unicode); }

void glfw_button(GLFWwindow *h, int button, int action, int mods) {
    window win = window::handle(h);
    switch (button) {
        case 0: win.w.buttons[ mouse::left  ] = action == GLFW_PRESS; break;
        case 1: win.w.buttons[ mouse::right ] = action == GLFW_PRESS; break;
        case 2: win.w.buttons[ mouse::middle] = action == GLFW_PRESS; break;
    }
    win.button_event(win.w.buttons);
}

void glfw_cursor (GLFWwindow *h, double x, double y) {
    window win = window::handle(h);
    win.w.cursor = vec2 { x, y };
    win.w.buttons[mouse::inactive] = !glfwGetWindowAttrib(h, GLFW_HOVERED);
    ///
    win.cursor_event({
        .pos         = vec2 { x, y },
        .buttons     = win.w.buttons
    });
}

void glfw_resize (GLFWwindow *handle, i32 w, i32 h) {
    window win = window::handle(handle);
    win.w.fn_resize();
    win.w.sz = size { h, w };
    win.resize_event({ .sz = win.w.sz });
}


GPU::operator VkPhysicalDevice() {
    return gpu;
}

bool GPU::operator()(Capability caps) {
    bool g = gfx.type()     == typeof(u32);
    bool p = present.type() == typeof(u32);
    if (caps == Complete) return g && p;
    if (caps == Graphics) return g;
    if (caps == Present)  return p;
    return true;
}

uint32_t GPU::index(Capability caps) {
    assert(gfx.type() == typeof(u32) || present.type() == typeof(u32));
    if (caps == Graphics && (*this)(Graphics)) return u32(gfx);
    if (caps == Present  && (*this)(Present))  return u32(present);
    assert(false);
    return 0;
}

GPU::operator bool() {
    return (*this)(Complete) && support.formats && support.present_modes && support.ansio;
}

/// Model is the integration of device, ubo, attrib, with interfaces around OBJ format or Lambda
template <typename V>
struct Model:Pipes {
    struct Polys {
        ::map<array<int32_t>> groups;
        array<V> verts;
    };
    
    ///
    struct Shape {
        typedef std::function<Polys(void)> ModelFn;
        ModelFn fn;
    };
    
    ///
    Model(Device &device) {
        //data = std::shared_ptr<Data>(new Data { &device });
    }
    
    static str form_path(str model, str skin, str ext) {
        str sk = skin ? str(skin + ".") : str("");
        return fmt {"textures/{0}{1}.{2}", {model, sk, ext}};
    }
    
    /// skin will just override where overriden in the file-system
    Assets cache_assets(str model, str skin, states<Asset> &atypes) {
        auto   &d     = *this->data;
        auto   &cache = d.device->tx_cache; /// ::map<Path, Device::Pair>
        Assets assets = Assets(Asset::count);
        ///
        auto load_tx = [&](Path p) -> Texture * {
            if (!cache.count(p)) {
                cache[p].img      = new image(p);
                cache[p].texture  = new Texture(d.device, *cache[p].img,
                   VK_IMAGE_USAGE_SAMPLED_BIT      | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false, VK_FORMAT_R8G8B8A8_UNORM, -1);
                cache[p].texture->push_stage(Texture::Stage::Shader);
            };
            return cache[p].texture;
        };
        
        #if 0
        static auto names = ::map<str> {
            { Asset::Color,    "color"    },
            { Asset::Specular, "specular" },
            { Asset::Displace, "displace" },
            { Asset::Normal,   "normal"   }
        };
        #endif

        doubly<memory*> &enums = Asset::symbols();
        ///
        for (memory *sym:enums) {
            str         name = sym->data<symbol>(); /// no allocation and it just holds onto this symbol memory, not mutable
            Asset type = Asset(sym->id);
            if (!atypes[type]) continue;
            /// prefer png over jpg, if either one exists
            Path png = form_path(model, skin, ".png");
            Path jpg = form_path(model, skin, ".jpg");
            if (!png.exists() && !jpg.exists()) continue;
            assets[type] = load_tx(png.exists() ? png : jpg);
        }
        return assets;
    }
    
    Model(str name, str skin, Device &device, UniformData &ubo, VAttribs &attr,
          states<Asset> &atypes, Shaders &shaders) : Model(device) {
        /// cache control for images to texture here; they might need a new reference upon pipeline rebuild
        auto assets = cache_assets(name, skin, atypes);
        auto  mpath = form_path(name, skin, ".obj");
        auto    obj = Obj<V>(mpath, [](auto& g, vec3& pos, vec2& uv, vec3& norm) {
            return V(pos, norm, uv, vec4f {1.0f, 1.0f, 1.0f, 1.0f});
        });
        auto &d = *this->data;
        // VertexBuffer(Device &device, array<V> &v, VAttribs &attr)
        d.vbo   = VertexBuffer<V>(device, obj.vbo, attr);
        for (field<obj::group> &fgroup: obj.groups) {
            symbol       name = fgroup.key;
            obj::group &group = fgroup.value;
            auto     ibo = IndexBuffer<uint32_t>(device, group.ibo);
            str   s_name = name;
            d.part[name] = Pipeline<V>(device, ubo, d.vbo, ibo, assets, rgba {0.0, 0.0, 0.0, 0.0}, shaders(s_name));
        }
    }
};

#if 0
void sk_canvas_gaussian(sk_canvas_data* sk_canvas, v2r* sz, r4r* crop) {
    SkPaint  &sk     = *sk_canvas->state->backend.sk_paint;
    SkCanvas &canvas = *sk_canvas->sk_canvas;
    ///
    SkImageFilters::CropRect crect = { };
    if (crop && crop->w > 0 && crop->h > 0) {
        SkRect rect = { SkScalar(crop->x),          SkScalar(crop->y),
                        SkScalar(crop->x + crop->w), SkScalar(crop->y + crop->h) };
        crect       = SkImageFilters::CropRect(rect);
    }
    sk_sp<SkImageFilter> filter = SkImageFilters::Blur(sks(sz->x), sks(sz->y), nullptr, crect);
    sk.setImageFilter(std::move(filter));
}
#endif

map<str> t_text_colors_8 = {
    { "#000000" , "\u001b[30m" },
    { "#ff0000",  "\u001b[31m" },
    { "#00ff00",  "\u001b[32m" },
    { "#ffff00",  "\u001b[33m" },
    { "#0000ff",  "\u001b[34m" },
    { "#ff00ff",  "\u001b[35m" },
    { "#00ffff",  "\u001b[36m" },
    { "#ffffff",  "\u001b[37m" }
};

map<str> t_bg_colors_8 = {
    { "#000000" , "\u001b[40m" },
    { "#ff0000",  "\u001b[41m" },
    { "#00ff00",  "\u001b[42m" },
    { "#ffff00",  "\u001b[43m" },
    { "#0000ff",  "\u001b[44m" },
    { "#ff00ff",  "\u001b[45m" },
    { "#00ffff",  "\u001b[46m" },
    { "#ffffff",  "\u001b[47m" }
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
    protected: /// gfx has access
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
    public:

    ///
    enums(state_change, defs,
       "defs, push, pop",
        defs, push, pop);
    
    struct cdata {
        size               sz;    /// size in integer units; certainly not part of the stack lol.
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
  ::size &size() { return m.sz; }

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
    inline operator bool() { return bool(m.sz); }
};

/// gfx: a frontend on skia
struct gfx:cbase {
    struct gdata {
        VkvgSurface    vg_surface;
        VkvgDevice     vg_device;
        VkvgContext    ctx;
        VkhDevice      vkh_device;
        VkhImage       vkh_image;
        str            font_default;
        window        *win;
        Texture        tx;
        draw_state    *ds;

        ~gdata() {
            if (ctx)        vkvg_destroy        (ctx);
            if (vg_device)  vkvg_device_destroy (vg_device);
            if (vg_surface) vkvg_surface_destroy(vg_surface);
        }
    } &g;

    inline ::window &window() { return *g.win; }
    inline Device   &device() { return *g.win->w.dev; }

    /// data is single instanced on this cbase, and the draw_state is passed in as type for the cbase, which atleast makes it type-unique
    ctr(gfx, cbase, gdata, g);

    /// create with a window (indicated by a name given first)
    gfx(::window &w);

    /// Quake2: { computer. updated. }
    void draw_state_change(draw_state *ds, cbase::state_change type) {
        g.ds = ds;
    }

    text_metrics measure(str text) {
        vkvg_text_extents_t ext;
        vkvg_font_extents_t tm;
        ///
        vkvg_text_extents(g.ctx, (symbol)text.cs(), &ext);
        vkvg_font_extents(g.ctx, &tm);
        return text_metrics::data {
            .w           =  real(ext.x_advance),
            .h           =  real(ext.y_advance),
            .ascent      =  real(tm.ascent),
            .descent     =  real(tm.descent),
            .line_height =  real(g.ds->line_height)
        };
    }

    str format_ellipsis(str text, real w, text_metrics &tm_result) {
        symbol       t  = (symbol)text.cs();
        text_metrics tm = measure(text);
        str result;
        ///
        if (tm->w <= w)
            result = text;
        else {
            symbol e       = "...";
            size_t e_len   = strlen(e);
            r32    e_width = measure(e)->w;
            size_t cur_w   = 0;
            ///
            if (w <= e_width) {
                size_t len = text.len();
                while (len > 0 && tm->w + e_width > w)
                    tm = measure(text.mid(0, int(--len)));
                if (len == 0)
                    result = str(cstr(e), e_len);
                else {
                    cstr trunc = (cstr)malloc(len + e_len + 1);
                    strncpy(trunc, text.cs(), len);
                    strncpy(trunc + len, e, e_len + 1);
                    result = str(trunc, len + e_len);
                    free(trunc);
                }
            }
        }
        tm_result = tm;
        return result;
    }

    void draw_ellipsis(str text, real w) {
        text_metrics tm;
        str draw = format_ellipsis(text, w, tm);
        if (draw)
            vkvg_show_text(g.ctx, (symbol)draw.cs());
    }

    void image(::image img, graphics::shape sh, vec2 align, vec2 offset, vec2 source) {
        attachment *att = img.find_attachment("vg-surf");
        if (!att) {
            VkvgSurface surf = vkvg_surface_create_from_bitmap(
                g.vg_device, (uint8_t*)img.pixels(), img.width(), img.height());
            att = img.attach("vg-surf", surf, [surf]() {
                vkvg_surface_destroy(surf);
            });
            assert(att);
        }
        VkvgSurface surf = (VkvgSurface)att->data;
        draw_state   &ds = *g.ds;
        ::size        sz = img.shape();
        r4r           &r = sh.rect();
        assert(surf);
        vkvg_set_source_rgba(g.ctx,
            ds.color->r, ds.color->g, ds.color->b, ds.color->a * ds.opacity);
        
        /// now its just of matter of scaling the little guy to fit in the box.
        v2<real> vsc = { math::min(1.0, r.w / real(sz[1])), math::min(1.0, r.h / real(sz[0])) };
        real      sc = (vsc.y > vsc.x) ? vsc.x : vsc.y;
        vec2     pos = { mix(r.x, r.x + r.w - sz[1] * sc, align.data.x),
                         mix(r.y, r.y + r.h - sz[0] * sc, align.data.y) };
        
        push();
        /// translate & scale
        translate(pos + offset);
        scale(sc);
        /// push path
        vkvg_rectangle(g.ctx, r.x, r.y, r.w, r.h);
        /// color & fill
        vkvg_set_source_surface(g.ctx, surf, source.data.x, source.data.y);
        vkvg_fill(g.ctx);
        pop();
    }

    void push() {
        cbase::push();
        vkvg_save(g.ctx);
    }

    void pop() {
        cbase::pop();
        vkvg_restore(g.ctx);
    }

    /// would be reasonable to have a rich() method
    /// the lines are most definitely just text() calls, it should be up to the user to perform multiline.
    ///
    /// ellipsis needs to be based on align
    /// very important to make the canvas have good primitive ops, to allow the user 
    /// to leverage high drawing abstracts direct from canvas, make usable everywhere!
    ///
    /// rect for the control area, region for the sizing within
    /// important that this info be known at time of output where clipping and ellipsis is concerned
    /// 
    void text(str text, graphics::rect rect, alignment align, vec2 offset, bool ellip) {
        draw_state &ds = *g.ds;
        rgba::data & c = ds.color;
        ///
        vkvg_save(g.ctx);
        vkvg_set_source_rgba(g.ctx, c.r, c.g, c.b, c.a * ds.opacity);
        ///
        v2r            pos  = { 0, 0 };
        text_metrics   tm;
        str            txt;
        ///
        if (ellip) {
            txt = format_ellipsis(text, rect->w, tm);
            /// with -> operator you dont need to know the data ref.
            /// however, it looks like a pointer.  the bigger the front, the bigger the back.
            /// in this case.  they equate to same operations
        } else {
            txt = text;
            tm  = measure(txt);
        }

        /// its probably useful to have a line height in the canvas
        real line_height = (-tm->descent + tm->ascent) / 1.66;
        v2r           tl = { rect->x,  rect->y + line_height / 2 };
        v2r           br = { rect->x + rect->w - tm->w,
                             rect->y + rect->h - tm->h - line_height / 2 };
        v2r           va = vec2(align);
        pos              = mix(tl, br, va);
        
        vkvg_show_text(g.ctx, (symbol)text.cs());
        vkvg_restore  (g.ctx);
    }

    void clip(graphics::shape cl) {
        draw_state  &ds = cur();
        ds.clip = cl;
        cl.vkvg_path(g.ctx);
        vkvg_clip(g.ctx);
    }

    Texture texture() { return g.tx; } /// associated with surface

    void flush() {
        vkvg_flush(g.ctx);
    }
    
    void clear(rgba c) {
        vkvg_save           (g.ctx);
        vkvg_set_source_rgba(g.ctx, c->r, c->g, c->b, c->a);
        vkvg_paint          (g.ctx);
        vkvg_restore        (g.ctx);
    }

    void font(::font f) {
        vkvg_select_font_face(g.ctx, f->alias.cs());
        vkvg_set_font_size   (g.ctx, f->sz);
    }

    void cap  (graphics::cap   c) { vkvg_set_line_cap (g.ctx, vkvg_line_cap_t (int(c))); }
    void join (graphics::join  j) { vkvg_set_line_join(g.ctx, vkvg_line_join_t(int(j))); }
    void translate(vec2       tr) { vkvg_translate    (g.ctx, tr->x, tr->y);  }
    void scale    (vec2       sc) { vkvg_scale        (g.ctx, sc->x, sc->y);  }
    void scale    (real       sc) { vkvg_scale        (g.ctx, sc, sc);        }
    void rotate   (real     degs) { vkvg_rotate       (g.ctx, radians(degs)); }
    void fill(graphics::shape  p) {
        p.vkvg_path(g.ctx);
        vkvg_fill(g.ctx);
    }

    void gaussian (vec2 sz, graphics::shape c) { }

    void outline  (graphics::shape p) {
        p.vkvg_path(g.ctx);
        vkvg_stroke(g.ctx);
    }

    void*    data      ()                   { return null; }
    str      get_char  (int x, int y)       { return str(); }
    str      ansi_color(rgba &c, bool text) { return str(); }

    ::image  resample  (::size sz, real deg, graphics::shape view, vec2 axis) {
        c4<u8> *pixels = null;
        int scanline = 0;
        return ::image(sz, (rgba::data*)pixels, scanline);
    }
};

struct terminal:cbase {
    static inline symbol reset_chr = "\u001b[0m";

    protected:
    struct tdata {
        array<glyph> glyphs;
        draw_state  *ds; /// store this because i dont think it will be optimized otherwise, too many lookups
    } &t; /// name these differently so you have uniquely named containers for data

    public:
    void draw_state_change(draw_state &ds, cbase::state_change type) {
        t.ds = &ds;
        switch (type) {
            case cbase::state_change::push:
                break;
            case cbase::state_change::pop:
                break;
            default:
                break;
        }
    }

    str ansi_color(rgba &c, bool text) {
        map<str> &map = text ? t_text_colors_8 : t_bg_colors_8;
        if (c->a < 32)
            return "";
        str hex = str("#") + str(c);
        return map.count(hex) ? map[hex] : "";
    }

    ctr(terminal, cbase, tdata, t);

    terminal(::size sz) : terminal()  {
        m.sz      = sz;
        m.pixel_t = typeof(char);  
        size_t n_chars = sz.area();
        t.glyphs = array<glyph>(sz);
        for (size_t i = 0; i < n_chars; i++)
            t.glyphs += glyph {};
    }

    void *data() { return t.glyphs.data(); }

    void set_char(int x, int y, glyph gl) {
        draw_state &ds = *t.ds;
      ::size sz = cbase::size();
        if (x < 0 || x >= sz[1]) return;
        if (y < 0 || y >= sz[0]) return;
        //assert(!ds.clip || sk_vshape_is_rect(ds.clip));
        size_t index  = y * sz[1] + x;
        t.glyphs[{y, x}] = gl;
        /*
        ----
        change-me: dont think this is required on 'set', but rather a filter on get
        ----
        if (cg.border) t.glyphs[index]->border  = cg->border;
        if (cg.chr) {
            str  gc = t.glyphs[index].chr;
            if ((gc == "+") || (gc == "|" && cg.chr == "-") ||
                               (gc == "-" && cg.chr == "|"))
                t.glyphs[index].chr = "+";
            else
                t.glyphs[index].chr = cg.chr;
        }
        if (cg.fg) t.glyphs[index].fg = cg.fg;
        if (cg.bg) t.glyphs[index].bg = cg.bg;
        */
    }

    // gets the effective character, including bordering
    str get_char(int x, int y) {
        cbase::draw_state &ds = *t.ds;
      ::size        &sz = m.sz;
        size_t       ix = math::clamp<size_t>(x, 0, sz[1] - 1);
        size_t       iy = math::clamp<size_t>(y, 0, sz[0] - 1);
        str       blank = " ";
        
        auto get_str = [&]() -> str {
            auto value_at = [&](int x, int y) -> glyph * {
                if (x < 0 || x >= sz[1]) return null;
                if (y < 0 || y >= sz[0]) return null;
                return &t.glyphs[y * sz[1] + x];
            };
            
            auto is_border = [&](int x, int y) {
                glyph *cg = value_at(x, y);
                return cg ? ((*cg)->border > 0) : false;
            };
            
            glyph::members &cg = t.glyphs[iy * sz[1] + ix];
            auto   t  = is_border(x, y - 1);
            auto   b  = is_border(x, y + 1);
            auto   l  = is_border(x - 1, y);
            auto   r  = is_border(x + 1, y);
            auto   q  = is_border(x, y);
            
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
            return cg.chr;
        };
        return get_str();
    }

    void text(str s, graphics::shape vrect, alignment::data &align, vec2 voffset, bool ellip) {
        r4<real>    rect   =  vrect.bounds();
      ::size       &sz      =  cbase::size();
        v2<real>   &offset =  voffset;
        draw_state &ds     = *t.ds;
        int         len    =  int(s.len());
        int         szx    = sz[1];
        int         szy    = sz[0];
        ///
        if (len == 0)
            return;
        ///
        int x0 = math::clamp(int(math::round(rect.x)), 0,          szx - 1);
        int x1 = math::clamp(int(math::round(rect.x + rect.w)), 0, szx - 1);
        int y0 = math::clamp(int(math::round(rect.y)), 0,          szy - 1);
        int y1 = math::clamp(int(math::round(rect.y + rect.h)), 0, szy - 1);
        int  w = (x1 - x0) + 1;
        int  h = (y1 - y0) + 1;
        ///
        if (!w || !h) return;
        ///
        int tx = align.x == xalign::left   ? (x0) :
                 align.x == xalign::middle ? (x0 + (x1 - x0) / 2) - int(std::ceil(len / 2.0)) :
                 align.x == xalign::right  ? (x1 - len) : (x0);
        ///
        int ty = align.y == yalign::top    ? (y0) :
                 align.y == yalign::middle ? (y0 + (y1 - y0) / 2) - int(std::ceil(len / 2.0)) :
                 align.y == yalign::bottom ? (y1 - len) : (y0);
        ///
        tx           = math::clamp(tx, x0, x1);
        ty           = math::clamp(ty, y0, y1);
        size_t ix    = math::clamp(size_t(tx), size_t(0), size_t(szx - 1));
        size_t iy    = math::clamp(size_t(ty), size_t(0), size_t(szy - 1));
        size_t len_w = math::  min(size_t(x1 - tx), size_t(len));
        ///
        for (size_t i = 0; i < len_w; i++) {
            int x = ix + i + offset.x;
            int y = iy + 0 + offset.y;
            if (x < 0 || x >= szx || y < 0 || y >= szy)
                continue;
            str     chr  = s.mid(i, 1);
            set_char(tx + int(i), ty,
                glyph::members {
                    0, chr, {0,0,0,0}, rgba::data(ds.color)
                });
        }
    }

    void outline(graphics::shape sh) {
        draw_state &ds = *t.ds;
        assert(sh);
        r4r     &r = sh.bounds();
        int     ss = math::min(2.0, math::round(ds.outline_sz));
        c4<u8>  &c = ds.color;
        glyph cg_0 = glyph::members { ss, "-", null, rgba { c.r, c.g, c.b, c.a }};
        glyph cg_1 = glyph::members { ss, "|", null, rgba { c.r, c.g, c.b, c.a }};
        
        for (int ix = 0; ix < int(r.w); ix++) {
            set_char(int(r.x + ix), int(r.y), cg_0);
            if (r.h > 1)
                set_char(int(r.x + ix), int(r.y + r.h - 1), cg_0);
        }
        for (int iy = 0; iy < int(r.h); iy++) {
            set_char(int(r.x), int(r.y + iy), cg_1);
            if (r.w > 1)
                set_char(int(r.x + r.w - 1), int(r.y + iy), cg_1);
        }
    }

    void fill(graphics::shape sh) {
        draw_state &ds = *t.ds;
      ::size       &sz = cbase::size();
        int         sx = sz[1];
        int         sy = sz[0];
        if (ds.color) {
            r4r &r       = sh.rect();
            str  t_color = ansi_color(ds.color, false);
            ///
            int      x0 = math::max(int(0),        int(r.x)),
                     x1 = math::min(int(sx - 1),   int(r.x + r.w));
            int      y0 = math::max(int(0),        int(r.y)),
                     y1 = math::min(int(sy - 1),   int(r.y + r.h));

            glyph    cg = glyph::members { 0, " ", rgba::data(ds.color), null }; /// members = data

            for (int x = x0; x <= x1; x++)
                for (int y = y0; y <= y1; y++)
                    set_char(x, y, cg); /// just set mem w grab()
        }
    }
};

export enums(operation, fill,
    "fill, image, outline, text, child",
     fill, image, outline, text, child);

export struct node:Element {
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
    T &context(str name) {
        node  *cur = (node*)this;
        while (cur) {
            type_t ctx = cur->mx::ctx;
            if (ctx) {
                prop *def = ctx->schema->meta_map.lookup(name); /// will always need schema
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
        if (std.content && std.content.type() == typeof(cstr)) {
            canvas.color(text.color);
            // (str text, graphics::rect area, region reg, alignment align, vec2 offset, bool ellip)
            // padding?
            // str text, graphics::rect rect, alignment align, vec2 offset, bool ellip
            canvas.text(str(std.content.grab()), text.shape.rect(), text.align, {0.0, 0.0}, true);
        }

        /// if there is an effective border to draw
        if (outline.color && outline.border.size() > 0.0) {
            canvas.color(outline.border.color());
            canvas.outline_sz(outline.border.size());
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

    inline size_t count(str n) {
        for (Element &c: *e.children)
            if (c.e.id == n)
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
            
            /// you just express requirements, and you get a boolean if its something that works with the class
            /// thats very intuitive feature
            constexpr bool transitions = requires(const T& t) { (t * real(0.6)) + (t * real(0.4)); };
            if constexpr (transitions) {
                real x = pos(value);
                real i = 1.0 - x;
                return (fr * i) + (to * x);
            } else
                return to;
        }

        /// construct transition with string, can be one of 3 sort of configurations
        /// none, 1.0061m cubic-in, 0.5s [default: none]
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

style::entry *prop_style(node &n, prop *member) {
    style           *st = (style *)n.fetch_style();
    mx         s_member = member->key;
    array<style::block> &blocks = st->members(s_member); /// instance function when loading and updating style, managed map of [style::block*]
    style::entry *match = null; /// key is always a symbol, and maps are keyed by symbol
    real     best_score = 0;
    ///
    /// find top style pair for this member
    for (style::block &block:blocks) {
        real score = block.match(&n);
        if (score > 0 && score >= best_score) {
            match = block.b_entry(member->key);
            best_score = score;
        }
    }
    
    return match;
}

bool ws(cstr &cursor) {
    auto is_cmt = [](symbol c) -> bool { return c[0] == '/' && c[1] == '*'; };
    while (isspace(*cursor) || is_cmt(cursor)) {
        while (isspace(*cursor))
            cursor++;
        if (is_cmt(cursor)) {
            cstr f = strstr(cursor, "*/");
            cursor = f ? &f[2] : &cursor[strlen(cursor) - 1];
        }
    }
    return *cursor != 0;
}

bool scan_to(cstr &cursor, array<char> chars) {
    bool sl  = false;
    bool qt  = false;
    bool qt2 = false;
    for (; *cursor; cursor++) {
        if (!sl) {
            if (*cursor == '"')
                qt = !qt;
            else if (*cursor == '\'')
                qt2 = !qt2;
        }
        sl = *cursor == '\\';
        if (!qt && !qt2)
            for (char &c: chars)
                if (*cursor == c)
                    return true;
    }
    return false;
}

doubly<style::qualifier> parse_qualifiers(cstr *p) {
    str   qstr;
    cstr start = *p;
    cstr end   = null;
    cstr scan  =  start;
    
    /// read ahead to {
    do {
        if (!*scan || *scan == '{') {
            end  = scan;
            qstr = str(start, std::distance(start, scan));
            break;
        }
    } while (*++scan);
    
    ///
    if (!qstr) {
        end = scan;
        *p  = end;
        return null;
    }
    
    ///
    auto  quals = qstr.split(",");
    doubly<style::qualifier> result;

    ///
    for (str &qs:quals) {
        str  q = qs.trim();
        if (!q) continue;
        style::qualifier &v = result.push();
        int idot = q.index_of(".");
        int icol = q.index_of(":");
        str tail;
        ///
        if (idot >= 0) {
            array<str> sp = q.split(".");
            v.data.type   = sp[0];
            v.data.id     = sp[1];
            if (icol >= 0)
                tail  = q.mid(icol + 1).trim();
        } else {
            if (icol  >= 0) {
                v.data.type = q.mid(0, icol);
                tail   = q.mid(icol + 1).trim();
            } else
                v.data.type = q;
        }
        array<str> ops {"!=",">=","<=",">","<","="};
        if (tail) {
            // check for ops
            bool is_op = false;
            for (str &op:ops) {
                if (tail.index_of(op.cs()) >= 0) {
                    is_op   = true;
                    auto sp = tail.split(op);
                    v.data.state = sp[0].trim();
                    v.data.oper  = op;
                    v.data.value = tail.mid(sp[0].len() + op.len()).trim();
                    break;
                }
            }
            if (!is_op)
                v.data.state = tail;
        }
    }
    *p = end;
    return result;
}

void style::cache_members() {
    lambda<void(block &)> cache_b;
    ///
    cache_b = [&](block &bl) -> void {
        for (entry &e: bl->entries) {
            bool  found = false;
            ///
            array<block> &cache = members(e->member);
            for (block &cb:cache)
                 found |= cb == bl;
            ///
            if (!found)
                 cache += bl;
        }
        for (block &s:bl->blocks)
            cache_b(s);
    };
    if (m.root)
        for (block &b:m.root)
            cache_b(b);
}

style::style(str code) : style(mx::alloc<style>()) {
    ///
    if (code) {
        for (cstr sc = code.cs(); ws(sc); sc++) {
            lambda<void(block)> parse_block;
            ///
            parse_block = [&](block bl) {
                ws(sc);
                console.test(*sc == '.' || isalpha(*sc), "expected Type, or .name");
                bl->quals = parse_qualifiers(&sc);
                ws(++sc);
                ///
                while (*sc && *sc != '}') {
                    /// read up to ;, {, or }
                    ws(sc);
                    cstr start = sc;
                    console.test(scan_to(sc, {';', '{', '}'}), "expected member expression or qualifier");
                    if (*sc == '{') {
                        ///
                        block &bl_n = bl->blocks.push();
                        bl_n->parent = bl;

                        /// parse sub-block
                        sc = start;
                        parse_block(bl_n);
                        assert(*sc == '}');
                        ws(++sc);
                        ///
                    } else if (*sc == ';') {
                        /// read member
                        cstr cur = start;
                        console.test(scan_to(cur, {':'}) && (cur < sc), "expected [member:]value;");
                        str  member = str(start, std::distance(start, cur));
                        ws(++cur);

                        /// read value
                        cstr vstart = cur;
                        console.test(scan_to(cur, {';'}), "expected member:[value;]");
                        
                        /// this should use the regex standard api, will convert when its feasible.
                        str  cb_value = str(vstart, std::distance(vstart, cur)).trim();
                        str       end = cb_value.mid(-1, 1);
                        bool       qs = cb_value.mid( 0, 1) == "\"";
                        bool       qe = cb_value.mid(-1, 1) == "\"";

                        if (qs && qe) {
                            cstr   cs = cb_value.cs();
                            cb_value  = str::parse_quoted(&cs, cb_value.len());
                        }

                        int         i = cb_value.index_of(",");
                        str     param = i >= 0 ? cb_value.mid(i + 1).trim() : "";
                        str     value = i >= 0 ? cb_value.mid(0, i).trim()  : cb_value;
                        style::transition trans = param  ? style::transition(param) : null;
                        
                        /// check
                        console.test(member, "member cannot be blank");
                        console.test(value,  "value cannot be blank");
                        bl->entries += entry::data { member, value, trans };
                        ws(++sc);
                    }
                }
                console.test(!*sc || *sc == '}', "expected closed-brace");
            };
            ///
            block &n_block = m.root.push_default();
            parse_block(n_block);
        }
        /// store blocks by member, the interface into all style: style::members[name]
        cache_members();
    }
}

style style::load(path p) {
    return cache.count(p) ? cache[p] : (cache[p] = style(p));
}

style style::for_class(symbol class_name) {
    path p = str::format("style/{0}.css", { class_name });
    return load(p);
}

typedef node* (*FnFactory)();

/*
    member data: (member would be map<style_value> )

    StyleValue
        NMember<T, MT> *host         = null; /// needs to be 
        transition     *trans        = null;
        bool            manual_trans = false;
        int64_t         timer_start  = 0;
        sh<T>           t_start;       // transition start value (could be in middle of prior transition, for example, whatever its start is)
        sh<T>           t_value;       // current transition value
        sh<T>           s_value;       // style value
        sh<T>           d_value;       // default value

    size_t        cache = 0;
    sh<T>         d_value;
    StyleValue    style;
*/

// we just need to make compositor with transitions a bit cleaner than previous attempt.  i want to use all mx-pattern


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
            states<Asset>   assets    = { Asset::color };
            Shaders         shaders   = { "*=main" };
            UniformData     ubo       = { null };
            VAttribs        attr      = { VA::position, VA::uv, VA::normal };
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
        m.plumbing = model<Vertex>(m.model, m.skin, m.ubo, m.attr, m.assets, m.shaders);
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
export struct button:node {
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
export struct list_view:node {
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
    
    array<Column>             columns;
    typedef array<Column>     Columns;
    
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

window::window(::size sz, mode::etype wmode, memory *control) : window() {
    /*
    static bool is_init = false;
    if(!is_init) {
        is_init = true;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    */
    VkInstance vk = Internal::handle().vk;
    w.sz          = sz;
    w.headless    = wmode == mode::headless;
    w.control     = control->grab();
    w.glfw        = glfwCreateWindow(sz[1], sz[0], (symbol)"ux-vk", null, null);
    w.vk_surface  = 0;
    
    VkResult code = glfwCreateWindowSurface(vk, w.glfw, null, &w.vk_surface);

    console.test(code == VK_SUCCESS, "initial surface creation failure.");

    /// set glfw callbacks
    glfwSetWindowUserPointer(w.glfw, (void*)mem->grab()); /// this is another reference on the window, which must be unreferenced when the window closes
    glfwSetErrorCallback    (glfw_error);

    if (!w.headless) {
        glfwSetFramebufferSizeCallback(w.glfw, glfw_resize);
        glfwSetKeyCallback            (w.glfw, glfw_key);
        glfwSetCursorPosCallback      (w.glfw, glfw_cursor);
        glfwSetCharCallback           (w.glfw, glfw_char);
        glfwSetMouseButtonCallback    (w.glfw, glfw_button);
    }

    array<VkPhysicalDevice> hw = Device::gpus();

    /// a fast algorithm for choosing top gpu
    const size_t top_pick = 0; 
    w.gpu = new GPU(hw[top_pick], w.vk_surface);

    /// create device with window, selected gpu and surface
    w.dev = new Device(*w.gpu, true); 
    w.dev->initialize(w);
}

gfx::gfx(::window &win) : gfx() {
    vk_interface vk; /// verify vulkan instanced prior
    g.win = &win;    /// should just set window without pointer
    m.sz  = win.w.sz;
    assert(m.sz[1] > 0 && m.sz[0] > 0);

    /// erases old main texture, returns the newly sized.
    /// this is 'texture-main' of sorts accessible from canvas context2D
    g.tx = win.texture(m.sz); 

    VkInstance vk_inst = vk.instance();
    u32   family_index = win.graphics_index();
    u32    queue_index = 0; // not very clear this.

  // not sure if the import is getting the VK_SAMPLE_COUNT_8_BIT and VkSampler, likely not.
    g.vg_device        = vkvg_device_create_from_vk_multisample(vk_inst, win.gpu(), win.device(), family_index, queue_index, VK_SAMPLE_COUNT_8_BIT, false);
    g.vkh_device       = vkh_device_import(vk.instance(), win.gpu(), win.device()); // use win instead of vk.
    g.vkh_image        = vkh_image_import(g.vkh_device, g.tx->vk_image, g.tx.data.format, g.tx->sz[1], g.tx->sz[0]);

    vkh_image_create_view(g.vkh_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT); // VK_IMAGE_ASPECT_COLOR_BIT questionable bit
    g.vg_surface       = vkvg_surface_create_for_VkhImage(g.vg_device, (void*)g.vkh_image);
    g.ctx              = vkvg_create(g.vg_surface);
    push(); /// gfxs just needs a push off the ledge. [/penguin-drops]
    defaults();
}

export struct composer:mx {
    ///
    struct data {
        node         *root;
        vk_interface *vk;
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

export struct app:composer {
    struct data {
        window win;
        gfx    canvas;
    } &m;

    ctr(app, composer, data, m);

    app(lambda<Element(app&)> app_fn) : app() { }
    ///
    int run() {
        for (size_t i = 0; i < 100; i++) {
            m.win    = window {size { 512, 512 }, mode::regular, mx::mem };
            m.canvas = gfx(m.win);
        }
        m.win    = window {size { 512, 512 }, mode::regular, mx::mem };
        m.canvas = gfx(m.win);
        Device &dev = m.canvas.device();
        window &win = m.canvas.window();

        ///
        auto   vertices = Vertex::square (); /// doesnt always get past this.
        auto    indices = array<uint16_t> { 0, 1, 2, 2, 3, 0 };
        auto   textures = Assets {{ Asset::Color, &win.texture() }}; /// quirk in that passing Asset::Color enumerable does not fetch memory with id, but rather memory of int(id)
        auto      vattr = VAttribs { VA::Position, VA::Normal, VA::UV, VA::Color };
        auto        vbo = VertexBuffer<Vertex>  { dev, vertices, vattr };
        auto        ibo = IndexBuffer<uint16_t> { dev, indices  };
        auto        uni = UniformBuffer<MVP>    { dev, [&](MVP &mvp) {
            mvp = MVP {
                 .model = m44f::ident(), //m44f::identity(),
                 .view  = m44f::ident(), //m44f::identity(),
                 .proj  = m44f::ortho({ -0.5f, 0.5f }, { -0.5f, 0.5f }, { 0.5f, -0.5f })
            };
        }}; /// assets dont seem to quite store Asset::Color and its texture ref
        auto         pl = Pipeline<Vertex> {
            dev, uni, vbo, ibo, textures,
            { 0.0, 0.0, 0.0, 0.0 }, std::string("main") /// transparent canvas overlay; are we clear? crystal.
        };

        ///
        win.show();
        
        /// prototypes add a Window&
        /// w.fn_cursor  = [&](double x, double y)         { composer->cursor(w, x, y);    };
        /// w.fn_mbutton = [&](int b, int a, int m)        { composer->button(w, b, a, m); };
        /// w.fn_resize  = [&]()                           { composer->resize(w);          };
        /// w.fn_key     = [&](int k, int s, int a, int m) { composer->key(w, k, s, a, m); };
        /// w.fn_char    = [&](uint32_t c)                 { composer->character(w, c);    };
        
        /// uniforms are meant to be managed by the app, passed into pipelines.
        win.loop([&]() {
            Element e = composer(&win);
            //canvas.clear(rgba {0.0, 0.0, 0.0, 0.0});
            //canvas.flush();

            // this should only paint the 3D pipeline, no canvas
            //i.tx_skia.push_stage(Texture::Stage::Shader);
            //device.render.push(pl); /// Any 3D object would have passed its pipeline prior to this call (in render)
            dev.render.present();
            //i.tx_skia.pop_stage();
            
            //if (composer->d.root) w.set_title(composer->d.root->m.text.label);

            // process animations
            //composer->process();
            
            //glfwWaitEventsTimeout(1.0);
        });
        glfwTerminate();
        return 0;
    }

    operator int() { return run(); }
};
