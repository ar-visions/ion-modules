#include <ux/ux.hpp>

bool Element::operator==(Element &b) {
    return factory == b.factory && args == b.args && elements == b.elements;
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

node::node(const char *selector, const char *cn, Args args, vec<Element> elements):
    selector(selector), class_name(cn), args(args), elements(elements) { }

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

void node::define_standard() {
    Define <str>     { this, "id",           &props.id,           ""};
    Define <str>     { this, "bind",         &props.bind,         ""};
    Define <int>     { this, "tab-index",    &props.tab_index,    -1};
    /// -------------------------------------------------------------------------
    Define <str>     { this, "text-label",   &props.text.label,   ""            };
    Define <Align>   { this, "text-align-x", &props.text.align.x, Align::Middle };
    Define <Align>   { this, "text-align-y", &props.text.align.y, Align::Middle };
    Define <rgba>    { this, "text-color",   &props.text.color,   rgba("#fff")  };
    /// -------------------------------------------------------------------------
    Define <rgba>    { this, "fill-color",   &props.fill.color,   null          };
    Define <double>  { this, "border-size",  &props.border.size,  double(0)     };
    Define <rgba>    { this, "border-color", &props.border.color, rgba("#000f") };
    Define <bool>    { this, "border-dash",  &props.border.dash,  bool(false)   };
    Define <Region>  { this, "region",       &props.region,       Region()      };
}

void node::define() { }
void node::mount() { }
void node::changed(PropList props) { }
void node::umount() { }
void node::input(str v) {
    /// when we do get tab keys, it should just be guaranteed to be a singular char
    if (v[0] == '\t') {
        node *sel = null;
        int target_index = props.tab_index;
        int lowest_diff  = -1;
        select([&](node *n) {
            if (n != this && n->props.tab_index >= 0) {
                int diff = n->props.tab_index - target_index;
                if (lowest_diff == -1 || lowest_diff > diff) {
                    lowest_diff = diff;
                    sel = n;
                }
            }
            return null;
        });
        if (sel)
            set_state("focus", sel);
    }
}

var node::get_state(std::string n) {
    node *h = ((n == "focus" || n == "capture") ? root : this);
    return h->smap.count(n) ? h->smap[n] : null;
}

var node::set_state(std::string n, var v) {
    node *h = ((n == "focus" || n == "capture") ? root : this);
    var prev = h->smap[n];
    h->smap[n] = v;
    return prev;
}

void node::draw(Canvas &canvas) {
    auto p = Path(rectd {0, 0, 32, 24});
    canvas.save();
    if (props.border.color && props.border.size > 0.0) {
        canvas.color(props.border.color);
        canvas.stroke_sz(props.border.size);
        auto p = path.rect; // offset border.
        p.x -= 1.0;
        p.y -= 1.0;
        p.w += 2.0;
        p.h += 2.0;
        canvas.stroke(p);
    }
    if (props.text) {
        canvas.color(props.text.color);
        canvas.text(props.text.label, path, props.text.align, {0.0, 0.0});
    }
    canvas.restore();
}

str node::format_text() {
    return props.text.label;
}

void delete_node(node *n) {
    delete n;
}

rectd node::calculate_rect(node *child) {
    return child->props.region(path);
}

rectd Region::operator()(node *n, node *rel) const {
    return (rel ? rel : n->root)->calculate_rect(n);
}
