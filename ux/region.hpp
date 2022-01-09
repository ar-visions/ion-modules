#pragma once
#include <dx/dx.hpp>
#include <dx/vec.hpp>

struct Node;
struct Region:io {
    enum Sides { L, T, R, B };
    vec<Unit>  sides;
    ///
    Region() { sides = vec<Unit> { "0l", "0t", "0r", "0b" }; }
    ///
    Region(Unit lside, Unit tside, Unit rside, Unit bside) {
        sides = vec<Unit> { lside, tside, rside, bside };
        assert_region();
    }
    ///
    Region(str st) {
        if (st) {
            auto a = st.split(" ");
            console.assertion(a.size() == 4, "expected: 4 sides");
            sides.reserve(4);
            for (int i = 0; i < 4; i++)
                sides += a[i];
            assert_region();
        } else
            sides = vec<Unit> { "0l", "0t", "0r", "0b" };
    }
    ///
    inline rectd operator()(rectd container) {
        if (!sides)
            return container;
        int index = int(L);
        real   x0 = container.x;
        real   y0 = container.y;
        real   x1 = container.x + container.w;
        real   y1 = container.y + container.h;
        for (auto &s:sides) {
            real  &v = s;
            switch (Sides(index++)) {
                case L: x0 = (s == "x" || s == "l") ? v : x1 - v; break;
                case T: y0 = (s == "y" || s == "t") ? v : y1 - v; break;
                case R: x1 = (s == "x" || s == "l") ? v : x1 - v; break;
                case B: y1 = (s == "y" || s == "t") ? v : y1 - v; break;
            }
        }
        return rectd { x0, y0, std::max(0.0, x1-x0), std::max(0.0, y1-y0) };
    }
    ///
    inline var exporter()       { return sides; }
    ///
    void importer(var& d)       { sides = vec<Unit>(d); }
    ///
    void copy(const Region &rg) { sides = rg.sides; }
    
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
