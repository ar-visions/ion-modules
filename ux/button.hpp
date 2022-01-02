#pragma once
#include <ux/ux.hpp>

struct Button:node {
    declare(Button);

    void draw(Canvas &canvas) {
        rectd r = rectd(path);
        canvas.color(m.fill.color);
        canvas.fill(r);
        canvas.color(m.text.color);
        canvas.scale(12.0);
        canvas.text(node::m.text.label, r, node::m.text.align, { 0, 0 }, true);
    }
};
