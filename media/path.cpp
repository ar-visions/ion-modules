#include <dx/dx.hpp>
#include <media/obj.hpp>
#include <core/SkPath.h>
#define IMPLEMENT
#include <media/path.hpp>

Path:: Path() : p(new SkPath())   { }
Path::~Path() { delete (SkPath *)p; }

void Path::move_to  (vec2 v)                   { p->moveTo ( v.x,  v.y); }
void Path::bezier_to(vec2 p0, vec2 p1, vec2 v) { p->cubicTo(p0.x, p0.y, p1.x, p1.y, v.x, v.y); }
void Path::quad_to  (vec2 q0, vec2 q1)         { p->quadTo (q0.x, q0.y, q1.x, q1.y); }
void Path::line_to  (vec2 v)                   { p->lineTo ( v.x,  v.y); }

void Path::arc_to   (vec2 c, double r, double d_from, double d, bool m) {
    if (d  > 360.0)
        d -= 360.0 * std::floor(d / 360.0);
    if (d >= 360.0)
        d  =   0.0;
    auto rect = SkRect { SkScalar(c.x - r), SkScalar(c.y - r),
                         SkScalar(c.x + r), SkScalar(c.y + r) };
    p->arcTo(rect, d_from, d, m);
}

Path::Path(const Path &ref) {
    if (&ref != this) {
         p = new SkPath();
        *p = *(ref.p);
    }
}

Path &Path::operator= (Path &ref) {
    if (&ref != this) {
         p = new SkPath();
        *p = *(ref.p);
    }
    return *this;
}

