#pragma once
#include <ux/ux.hpp>

/// bind-compatible List component for displaying data from models or static (type cases: array)
struct List:node {
    declare(List);
    
    struct Column:io {
        str    id     = null;
        double final  = 0;
        double size   = 1.0;
        bool   scale  = true;
        Align  align  = Align::Start;
        
        Column() { }
        
        Column(str id, double size = 1.0, Align align = Align::Start) :
            id(id), size(size),  scale(true),  align(align) { }
        
        Column(str id, int value, Align align = Align::Start) :
            id(id), size(value), scale(false), align(align) { }
        
        void import_data(var &d) {
            if (d == var::Array) {
                id     =             str(d[size_t(0)]);
                size   =          double(d[size_t(1)]);
                scale  =            bool(d[size_t(2)]);
                final  =          double(d[size_t(3)]);
                align  = Align::Type(int(d[size_t(4)]));
            } else {
                assert(d == var::Str);
                id = str(d);
            }
        }
        
        void copy(const Column &r) {
            id          = r.id;
            size        = r.size;
            scale       = r.scale;
            final       = r.final;
            align       = r.align;
        }
        
        var export_data() {
            return std::vector<var> { id, size, scale, final, align };
        }
        
        serializer(Column, id);
    };
    
    vec<Column>             columns;
    typedef vec<Column>     Columns;
    
    struct Members {
        struct Cell {
            Extern<str>         id;
            Extern<str>         text;
            Extern<rgba>        fg;
            Extern<rgba>        bg;
            Extern<Vec2<Align>> va;
        } cell;
        struct ColumnConf:Cell {
            Extern<Columns>     ids;
        } column;
        Extern<rgba>        odd_bg;
    } m;
    
    void bind() {
        external<rgba>    ("cell-fg",    m.cell.fg,    rgba {null});
        external<rgba>    ("cell-bg",    m.cell.bg,    rgba {null});
        external<rgba>    ("odd-bg",     m.odd_bg,     rgba {null});
        external<rgba>    ("column-fg",  m.column.fg,  rgba {"#0f0"});
        external<rgba>    ("column-bg",  m.column.bg,  rgba {"#00f"});
        external<Columns> ("column-ids", m.column.ids, {});
    }
    
    void update_columns() {
        columns      = vec<Column>(m.column.ids());
        double tsz   = 0;
        double warea = node::path.rect.w;
        double t     = 0;
        ///
        for (auto &c: columns)
            if (c.scale)
                tsz += c.size;
        ///
        for (auto &c: columns)
            if (c.scale) {
                c.size /= tsz;
                t += c.size;
            } else
                warea = std::max(0.0, warea - c.size);
        ///
        for (auto &c: columns)
            if (c.scale)
                c.final = std::round(warea * (c.scale / t));
            else
                c.final = c.size;
    }
    
    /*
    void draw(Canvas &canvas) {
        update_columns();
        node::draw(canvas);
        auto data = context(node::m.bind);
        if (!data.size())
            return;
        rectd r = {path.rect.x, path.rect.y, path.rect.w, 1.0};
        ///
        /// would be nice to just draw 1 stroke here, canvas should work with that.
        auto draw_rect = [&](rgba &c, rectd r) {
            Path path = r; // todo: decide on final xy scaling for text and gfx; we need a the same unit scales
            canvas.save();
            canvas.color(c);
            canvas.stroke(path);
            canvas.restore();
        };
        ///
        Column nc = null;
        /// would be useful to have a height param here
        auto cells = [&] (std::function<void(Column &, rectd &)> fn) {
            if (columns) {
                vec2 p = { r.x, r.y };
                for (Column &c: columns) {
                    double w = c.final;
                    rectd cell = {p.x, r.y, w - 1.0, 1.0};
                    fn(c, cell);
                    p.x += w;
                }
            } else
                fn(nc, r);
        };
        ///
        auto pad_rect = [&](rectd r) -> rectd {
            return {r.x + 1.0, r.y, r.w - 2.0, r.h};
        };
        /// paint column text, fills and strokes
        if (columns) {
            bool prev = false;
            Border &border = node::m.border;
            cells([&](Column &c, rectd cell) {
                var d_id = c.id;
                auto text = context("resolve")(d_id);
                canvas.color(node::m.text.ref().color);
                canvas.text(text, pad_rect(cell), {c.align, Align::Middle}, {0,0});
                if (prev)
                    draw_rect(border.color,
                        rectd {cell.x, cell.y - border.size, 1.0, path.rect.h + (border.size * 2)});
                else
                    prev = true;
            });
            draw_rect(border.color, {r.x - 1.0, r.y + 1.0, r.w + 2.0, 0.0});
            r.y += 2.0;
        }
        
        /// paint visible rows
        auto  &d_array = *(data.a);
        Text  &text    = node::m.text;
        int    limit   = y_scroll + path.rect.h - (columns ? 2.0 : 0.0);
        for (int i = y_scroll; i < int(d_array.size()); i++) {
            if (i >= limit)
                break;
            auto &d = d_array[i];
            cells([&](Column &c, rectd cell) {
                str s = c ? str(d[c.id.s]) : str(d);
                rgba &clr = (i & 1) ? m.odd_bg : m.cell.bg;
                canvas.color(clr);
                canvas.fill(cell);
                canvas.color(text.color);
                canvas.text(s, pad_rect(cell), {c.align, Align::Middle}, {0,0});
            });
            r.y += r.h;
        }
    }*/
};
