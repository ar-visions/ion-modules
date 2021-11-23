#include <dx/dx.hpp>
#include <media/obj.hpp>

struct Path {
private:
    SkPath *p;
public:
    Path();
   ~Path();
    
    void move_to  (vec2 v);
    void bezier_to(vec2 p0, vec2 p1, vec2 v);
    void quad_to  (vec2 q0, vec2 q1);
    void line_to  (vec2 v);
    void arc_to   (vec2 c, double r, double d_from, double d, bool m);
    Path(const Path &ref);
    Path &operator= (Path &ref);
    
    // todo:
    // the trick here is skia is a bit coy in its interface so we would need to have a
    // caching system where a render of the data happens on draw or serialize
    // operator var();
    // Path(var &d);
};
