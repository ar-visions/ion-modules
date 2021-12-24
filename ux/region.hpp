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
    serializer(Coord, t > Undefined);
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
    
    void importer(var& d) {
        if (d.size() >= 12) {
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
    serializer(Region, left);
};
