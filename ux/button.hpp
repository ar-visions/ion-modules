#pragma once
#include <ux/ux.hpp>

struct Button:node {
    declare(Button);

    /// put all of this into node::draw() but i think thats there already.
    void draw(Canvas &canvas) {
        canvas.color(m.fill.color);
        canvas.fill(paths.fill);
        canvas.color(m.text.color);
        canvas.scale(1.0);
        canvas.text(node::m.text.label, paths.border, node::m.text.align, { 0, 0 }, true);
    }
};
