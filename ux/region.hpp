#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>

struct Node;
struct Region:io {
    enum Sides { L, T, R, B };
    array<Unit>  sides;
    ///
    Region() { sides = array<Unit> { "0l", "0t", "0r", "0b" }; }
    ///
    Region(Unit lside, Unit tside, Unit rside, Unit bside) {
        sides = array<Unit> { lside, tside, rside, bside };
        assert_region();
    }
    ///
    Region(str st) {
        if (st) {
            auto a = st.split(" ");
            if (a.size() == 1) { /// 20p would fill an object in its parent with 20 padding around
                Unit ua = Unit(a[0]);
                real  u = ua;
                console.assertion(!std::isnan(u),        "invalid unit read in region");
                console.assertion(ua == "" || ua == "p", "invalid pad unit");
                str s0 = var::format("{0}l", {real(u)});
                str s1 = var::format("{0}t", {real(u)});
                str s2 = var::format("{0}r", {real(u)});
                str s3 = var::format("{0}b", {real(u)});
                sides = {s0, s1, s2, s3};
                ///
                /// additional states to implement:
                ///     parent-hover
                ///     child-hover
                ///     parent-focus
                ///     child-focus
                ///     cursor, an internal needs to be read-only in style expression
                ///         gradient-pos: [expression_name] // Unit to read these expressions?
                ///
            } else {
                console.assertion(a.size() == 4, "pad unit or 4 sides");
                sides.reserve(4);
                for (int i = 0; i < 4; i++)
                    sides += a[i];
                assert_region();
            }
        } else
            sides = array<Unit> { "0l", "0t", "0r", "0b" };
    }
    ///
    inline rectd operator()(rectd ref) {
        if (!sides)
            return ref;
        int index = 0;
        real   x0 = ref.x;
        real   y0 = ref.y;
        real   x1 = ref.x + ref.w;
        real   y1 = ref.y + ref.h;
        for (auto &s:sides) {
            real  &v = s;
            switch (index++) {
                case L:  x0 = (s == "x" || s == "l") ? x0 + v : x1 - v; break;
                case T:  y0 = (s == "y" || s == "t") ? y0 + v : y1 - v; break;
                case R:  x1 = (s == "w") ? x0 + v : ((s == "x" || s == "l") ? x0 + v : x1 - v); break;
                case B:  y1 = (s == "h") ? y0 + v : ((s == "y" || s == "t") ? y0 + v : y1 - v); break;
                default: assert(false);
            }
        }
        return rectd { x0 - ref.x, y0 - ref.y, std::max(0.0, x1-x0), std::max(0.0, y1-y0) };
    }
    ///
    inline var exporter()       { return sides; }
    ///
    void importer(var& d)       { sides = array<Unit>(d); }
    ///
    void copy(const Region &rg) {
        sides = rg.sides;
    }
    
    void assert_region() {
        auto &s = sides;
        console.assertion(s.size() == 4, "region requires 4 sides: xlr ytb lrw tbh");
        console.assertion(s[L] == "x" || s[L] == "l" || s[L] == "r",
                          "l-side coordinate invalid unit: {0}", {s[L]});
        console.assertion(s[T] == "y" || s[T] == "t" || s[T] == "b",
                          "t-side coordinate invalid unit: {0}", {s[T]});
        console.assertion(s[R] == "x" || s[R] == "l" || s[R] == "r" || s[R] == "w",
                          "r-side coordinate invalid unit: {0}", {s[R]});
        console.assertion(s[B] == "y" || s[B] == "t" || s[B] == "b" || s[B] == "h",
                          "b-side coordinate invalid unit: {0}", {s[B]});
    }
    ///
    io_shim(Region, true);
};

template<> struct is_strable<Region> : std::true_type {};
