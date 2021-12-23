#include <ux/ux.hpp>

bool Element::operator==(Element &b) {
    return factory == b.factory && binds == b.binds && elements == b.elements;
}

bool Element::operator!=(Element &b) {
    return !operator==(b);
}

Element::Element(var &ref, FnFilterArray f) : fn_filter_arr(f) { }
Element::Element(var &ref, FnFilterMap f)   : fn_filter_map(f) { }
Element::Element(var &ref, FnFilter f)      : fn_filter(f)     { }

Element Element::each(var &d, FnFilter f) {
    return Element(d, f);
}

Element Element::each(var &d, FnFilterArray f) {
    return Element(d, f);
}

Element Element::each(var &d, FnFilterMap f) {
    return Element(d, f);
}

node *node::select_first(std::function<node *(node *)> fn) {
    std::function<node *(node *)> recur;
    recur = [&](node* n) -> node* {
        node* r = fn(n);
        if   (r) return r;
        for (auto &[id, sn]: n->mounts) {
            if (!sn) continue;
            node* r = recur(sn);
            if   (r) return r;
        }
        return null;
    };
    return recur(this);
}

vec<node *> node::select(std::function<node *(node *)> fn) {
    vec<node *> result;
    std::function<node *(node *)> recur;
    recur = [&](node* n) -> node* {
        node* r = fn(n);
        if   (r) result += r;
        for (auto &[id, sn]:n->mounts)
            if (sn)
                recur(sn);
        return null;
    };
    recur(this);
    return result;
}

node::node(const char *selector, const char *cn, Binds binds, vec<Element> elements):
    selector(selector), class_name(cn), elements(elements), binds(binds) { }

node::node(nullptr_t n) { }

Element node::render() {
    return Element(this); // default render behaviour, so something like a Group, or a Button
}

bool node::processing() {
    return !!select_first([&](node *n) { return n->queue.size() ? n : null; });
}

bool node::process() {
    bool active = false;
    var zero;
    assert(!parent);
    for (auto &[id, n]:mounts) {
        if (!n)
            continue;
        n->select([&](node *n) {
            auto c = n->queue.size();
            while (c--) {
                n->queue.front()(zero);
                n->queue.pop();
            }
            active |= n->queue.size() > 0;
            return null;
        });
    }
    return active;
}

node::~node() { }

void node::standard() {
    contextual<str>  ("id", m.id, "");
    /// -------------------------------------------------------------------------
    external <str>   ("bind",         m.bind,         "");
    external <int>   ("tab-index",    m.tab_index,    -1);
    /// -------------------------------------------------------------------------
    external <str>   ("text-label",   m.text.label,   "");
    external <AlignV2> ("text-align", m.text.align,  AlignV2 { Align::Middle, Align::Middle });
    external <rgba>  ("text-color",   m.text.color,   rgba("#fff") );
    /// -------------------------------------------------------------------------
    external <rgba>  ("fill-color",   m.fill.color,   null);
    external <double>("border-size",  m.border.size,  double(0));
    external <rgba>  ("border-color", m.border.color, rgba("#000f"));
    external <bool>  ("border-dash",  m.border.dash,  bool(false));
    external <Region>("region",       m.region,       Region());
}

void node::bind() { }
void node::mount() { }
void node::changed(PropList props) { }
void node::umount() { }
void node::input(str v) {
    if (v[0] == '\t') {
        node        *sel = null;
        int target_index = m.tab_index;
        int  lowest_diff = -1;
        select([&](node *n) {
            if (n != this && n->m.tab_index() >= 0) {
                int diff = n->m.tab_index() - target_index;
                if (lowest_diff == -1 || lowest_diff > diff) {
                    lowest_diff = diff;
                    sel = n;
                }
            }
            return null;
        });
        if (sel)
            sel->focus();
    }
}

void  node::focus()   { flags |=  Focused; }
void  node::blur()    { flags &= ~Focused; }
node *node::focused() {
    std::function<node *(node *)> fn;
    return (fn = [&](node *n) -> node * {
        if (n->flags & Focused)
            return n;
        for (auto &[k, n]: n->mounts) {
            node *r = fn(n);
            if (r)
                return r;
        }
        return null;
    })(this);
}

void node::draw(Canvas &canvas) {
    auto p = Path(rectd {0, 0, 32, 24}); // ?
    canvas.save();
    Border &b = m.border;
    if (b.color() && b.size() > 0.0) {
        canvas.color(b.color());
        canvas.stroke_sz(b.size());
        auto p = path.rect; // offset border.
        p.x -= 1.0;
        p.y -= 1.0;
        p.w += 2.0;
        p.h += 2.0;
        canvas.stroke(p);
    }
    if (m.text) {
        canvas.color(m.text.color);
        canvas.text(m.text.label, path, m.text.align, {0.0, 0.0});
    }
    canvas.restore();
}

str node::format_text() {
    return m.text.label;
}

void delete_node(node *n) {
    delete n;
}

rectd node::calculate_rect(node *child) {
    return child->m.region()(path);
}

rectd Region::operator()(node *n, node *rel) const {
    return (rel ? rel : n->root)->calculate_rect(n);
}
