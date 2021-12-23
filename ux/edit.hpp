#pragma once
#include <ux/ux.hpp>

struct Edit:node {
    declare(Edit);
    struct Members {
        Extern<bool>     word_wrap;
        Extern<bool>     multi_line;
        Extern<bool>     secret;
        Extern<int>      max_len;
        Extern<str>      blank_text;
        Extern<FnFilter> filter;
        Extern<rgba>     sel_bg;
        Extern<rgba>     sel_fg;
    } m;
    
    static FnFilter LCase() {
        return FnFilter([](var &d) {
            std::string s = *(d.s);
            std::transform(s.begin(), s.end(), s.begin(), [](uint8_t c) { return std::tolower(c); });
            return var(s);
        });
    }
    
    void bind() {
        external<bool>     ("word-wrap",  m.word_wrap,  false);
        external<bool>     ("multi-line", m.multi_line, false);
        external<bool>     ("secret",     m.secret,     false);
        external<rgba>     ("sel-fg",     m.sel_fg,     rgba {"#ff0"});
        external<rgba>     ("sel-bg",     m.sel_bg,     rgba {"#000"});
        external<int>      ("max-len",    m.max_len,    -1);
        external<str>      ("blank-text", m.blank_text, "");
        external<FnFilter> ("filter",     m.filter,     FnFilter {null});
    }
    
    void draw(Canvas &canvas) {
        node::draw(canvas);
        Text &text = node::m.text;
        canvas.text(text.label, path.rect, text.align, {0,0});
    }
};
