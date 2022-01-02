#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>

struct Coord:io {
    enum Type {
        Undefined,
        Left,
        Top,
        Right,
        Bottom,
        Width,
        Height
    } t = Undefined;
    ///
    double v;
    bool   p;
    ///
    Coord(Type t = Undefined, double v = 0.0) : t(t),     v(v),  p(true)  { }
    Coord(Type t, int v)                      : t(t), v(int(v)), p(false) { }
    Coord(Type t, double v, bool p)           : t(t),     v(v),  p(p)     { }
    ///
    double operator()(double from, double size, double rel = 0.0) const {
        const double     s = p ? size : 1.0;
        const bool use_rel = t == Width || t == Height;
        if (t == Left || t == Top || use_rel)
            return (use_rel ? rel : 0) + from + (v * s);
        else
            return from + size - (v * s);
    }
    bool operator==(Type t) {
        return this->t == t;
    }
    void importer(var &d) {
        t = Type(double(d[size_t(0)]));
        v =      double(d[size_t(1)]);
        p =        bool(d[size_t(2)]);
    }
    inline var exporter() {
        return std::vector<double> { double(t), double(v), double(p) };
    }
    void copy(const Coord &ref) {
        t = ref.t;
        v = ref.v;
        p = ref.p;
    }
    io_define(Coord, t > Undefined);
};

struct L:Coord { L(double v) : Coord(Left,   v) { };
                 L(int    v) : Coord(Left,   v) { }};
struct T:Coord { T(double v) : Coord(Top,    v) { };
                 T(int    v) : Coord(Top,    v) { }};
struct R:Coord { R(double v) : Coord(Right,  v) { };
                 R(int    v) : Coord(Right,  v) { }};
struct B:Coord { B(double v) : Coord(Bottom, v) { };
                 B(int    v) : Coord(Bottom, v) { }};
struct W:Coord { W(double v) : Coord(Width,  v) { };
                 W(int    v) : Coord(Width,  v) { }};
struct H:Coord { H(double v) : Coord(Height, v) { };
                 H(int    v) : Coord(Height, v) { }};

struct Node;
struct Region:io {
    enum Type {
        Undefined,
        Left,
        Top,
        Right,
        Bottom
    };
    Coord left = L(0), top = T(0), right = R(0), bottom = B(0);
    
    Region() { }
    Region(Coord left, Coord top, Coord right, Coord bottom) :
        left(left),   top(top),
        right(right), bottom(bottom) {
        assert(left   == Coord::Left || left   == Coord::Right);
        assert(top    == Coord::Top  || top    == Coord::Bottom);
        assert(right  == Coord::Left || right  == Coord::Right  || right  == Coord::Width);
        assert(bottom == Coord::Top  || bottom == Coord::Bottom || bottom == Coord::Height);
    }
    
    inline rectd operator()(rectd rt) const {
        auto vx0 =   left(rt.x, rt.w, 0.0);
        auto vy0 =    top(rt.y, rt.h, 0.0);
        auto vx1 =  right(rt.x, rt.w, vx0);
        auto vy1 = bottom(rt.y, rt.h, vy0);
        return Rect<double>(vec2 { vx0, vy0 }, vec2 { vx1, vy1 });
    }
    
    inline var exporter() {
        var dd = std::vector<double> {
            double(left.t),   double(left.v),   double(left.p),
            double(top.t),    double(top.v),    double(top.p),
            double(right.t),  double(right.v),  double(right.p),
            double(bottom.t), double(bottom.v), double(bottom.p)
        };
        return dd;
    }
    /*
    struct UValue {
        str  unit;
        real value;
    };
    
    vec<UValue> split_units(str &s) {
        char *start = s.cstr();
        char *sc    = start;
        bool  find  = true;
        char letter = 0;
        while (*sc) {
            while (isspace(*sc))
                sc++;
            char    c = *sc;
            bool is_d = c == '-' || (c >= '0' && c <= '9');
            if (!find && isalpha(c)) {
                letter = c;
                find   = false;
            }
        };
    }*/
    
    void importer(var& d) {
        if (d == ::Type::Str) {
            auto   parse_side = [](str &s, enum Coord::Type def_side, Coord &e)  {
                auto word_letter = [](str &s) -> str {
                    bool listen = true;
                    for (const char c:s.s) {
                        if (listen && isalpha(c)) { /// it should take the first letter, preferring start but should also take a suffix too
                            char b[2] = { char(toupper(c)), 0 };
                            return str(b, 2);
                        } else
                            listen = isspace(c);
                    }
                    return str(null);
                };
                auto   p_side = Coord::Undefined;
                static auto m = map<const char *, Coord::Type> {
                    {"L", Coord::Left},  {"T", Coord::Top},
                    {"X", Coord::Left},  {"Y", Coord::Top},
                    {"R", Coord::Right}, {"B", Coord::Bottom},
                    {"W", Coord::Width}, {"H", Coord::Height}
                };
                ///
                for (auto &[k,v]: m)
                    if (word_letter(s) == k)
                        p_side = v;
                ///
                real     v = s.real();
                bool     p = s.index_of("%") >= 0;
                ///
                e = Coord((p_side == Coord::Undefined) ? def_side : p_side, v, p);
            };
            ///
            auto sp = str(d).split(" ");
            assert(sp.size() >= 4);
            ///
            parse_side(sp[0], Coord::Left,   left);
            parse_side(sp[1], Coord::Top,    top);
            parse_side(sp[2], Coord::Right,  right);
            parse_side(sp[3], Coord::Bottom, bottom);
            
            /// let us help each other
            if ((left == Coord::Top  || left == Coord::Bottom || left == Coord::Height) &&
                (top  == Coord::Left || top  == Coord::Right  || top  == Coord::Width))
                std::swap(left, top);
            if ((right  == Coord::Top  || right  == Coord::Bottom || right  == Coord::Height) &&
                (bottom == Coord::Left || bottom == Coord::Right  || bottom == Coord::Width))
                std::swap(right, bottom);
            
        } else if (d.size() >= 12) {
            std::vector<var> &a = *(d.a);
            left    = Coord { Coord::Type(double(a[size_t(0)])), double(a[size_t(1)]),  bool(double(a[size_t(2)]))  };
            top     = Coord { Coord::Type(double(a[size_t(3)])), double(a[size_t(4)]),  bool(double(a[size_t(5)]))  };
            right   = Coord { Coord::Type(double(a[size_t(6)])), double(a[size_t(7)]),  bool(double(a[size_t(8)]))  };
            bottom  = Coord { Coord::Type(double(a[size_t(9)])), double(a[size_t(10)]), bool(double(a[size_t(11)])) };
        }
    }
    
    void copy(const Region &rg) {
        left   = rg.left;
        top    = rg.top;
        right  = rg.right;
        bottom = rg.bottom;
    }
    
    rectd operator()(node *n, node *rel) const;
    io_define(Region, left);
};
