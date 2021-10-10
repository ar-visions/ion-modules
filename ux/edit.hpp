#pragma once
#include <ux/ux.hpp>

struct Edit:node {
    declare(Edit);
    struct Props:IProps {
        bool        word_wrap;
        bool        multi_line;
        bool        secret;
        int         max_len;
        str         blank_text;
        FnFilter    filter;
        rgba        sel_bg;
        rgba        sel_fg;
    } props;
    
    static FnFilter LCase() {
        return FnFilter([](Data &d) {
            std::string s = *(d.s);
            std::transform(s.begin(), s.end(), s.begin(), [](uint8_t c) { return std::tolower(c); });
            return Data(s);
        });
    }
    
    void define() {
        Define {this, "word-wrap",  &props.word_wrap,  false};
        Define {this, "multi-line", &props.multi_line, false};
        Define {this, "secret",     &props.secret,     false};
        Define {this, "sel-bg",     &props.sel_bg,     rgba {"#ff0"}};
        Define {this, "sel-fg",     &props.sel_fg,     rgba {"#000"}};
        Define {this, "max-len",    &props.max_len,    -1};
        Define {this, "blank-text", &props.blank_text, str {"#00f"}};
        Define {this, "filter",     &props.filter,     FnFilter {null}};
    }
    
    void draw(Canvas &canvas) {
        node::draw(canvas);
        auto &data = context(node::props.bind);
        if (!data.size())
            return;
        auto  text = str(data);
        /// implement binding api
        auto draw_line = [&](str &s) {
            canvas.text(s, path.rect, node::props.text.align, {0,0});
        };
        canvas.save();
        draw_line(text);
        // draw all of that selection stuff.
        canvas.restore();
    }
};
