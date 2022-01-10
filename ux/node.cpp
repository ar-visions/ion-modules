#include <ux/ux.hpp>

// fn_type_set not called via update, and this specific call is only when its assigned by binding
void Member::operator=(Member &v) {
    if (type == v.type)
        fn->type_set(*this, v.ptr());
    else {
        /// simple conversions here in constexpr for primitives; no need to goto var when its an easy convert
        assert(v.fn->var_get); /// must inherit from io
        assert(v.fn->var_set);
        var val = v.fn->var_get(*this);
        fn->var_set(*this, val);
    }
    bound_to   = &v;
    peer_cache = v.cache; // an external can pass on style when bound to anotehr external (and not set explicitly), but not internal -> external
    /// must flag for style update
    node->flags |= node::StateUpdate;
}

bool Element::operator==(Element &b) {
    return factory == b.factory && binds == b.binds && elements == b.elements;
}

bool Element::operator!=(Element &b) {
    return !operator==(b);
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

void node::exec(std::function<void(node *)> fn) {
    std::function<node *(node *)> recur;
    recur = [&](node* n) -> node* {
        fn(n);
        for (auto &[id, sn]:n->mounts)
            if (sn)
                recur(sn);
        return null;
    };
    recur(this);
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
    return Element(this); // standard render end-point
}

bool node::processing() {
    return !!select_first([&](node *n) { return n->queue.size() ? n : null; });
}

bool node::process() {
    bool active = (flags & node::StyleAnimate) != 0;
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
    flags &= ~node::StyleAnimate;
    return active;
}

node::~node() { }

void node::standard_bind() {
    contextual<str>        ("id",               m.id,              str(""));
    /// --------------------------------------------------------------------------
    external <int>         ("tab-index",        m.tab_index,       -1);
    /// --------------------------------------------------------------------------
    external <str>         ("text-label",       m.text.label,      str(""));
    external <AlignV2>     ("text-align",       m.text.align,      AlignV2 { Align::Middle, Align::Middle });
    external <rgba>        ("text-color",       m.text.color,      rgba("#fff") );
    external <real>        ("text-offset",      m.text.offset,     real(0));
    /// --------------------------------------------------------------------------
    external <rgba>        ("fill-color",       m.fill.color,      null);
    external <real>        ("fill-offset",      m.fill.offset,     real(0));
    external <Image>       ("fill-image",       m.fill.image,      null);
    external <double>      ("border-size",      m.border.size,     double(0));
    external <rgba>        ("border-color",     m.border.color,    rgba("#000f"));
    external <bool>        ("border-dash",      m.border.dash,     bool(false));
    external <real>        ("border-offset",    m.border.offset,   real(0));
    external <Cap>         ("border-cap",       m.border.cap,      Cap(Cap::Round));
    external <Join>        ("border-join",      m.border.join,     Join(Join::Round));
    external <Region>      ("region",           m.region,          Region());
    /// --------------------------------------------------------------------------
    external <Fn>          ("ev-hover",         m.ev.hover,        null);
    external <Fn>          ("ev-out",           m.ev.out,          null);
    external <Fn>          ("ev-down",          m.ev.down,         null);
    external <Fn>          ("ev-up",            m.ev.up,           null);
    external <Fn>          ("ev-key",           m.ev.key,          null);
    external <Fn>          ("ev-focus",         m.ev.focus,        null);
    external <Fn>          ("ev-blur",          m.ev.blur,         null);
    /// --------------------------------------------------------------------------
    external <real>        ("radius",           m.radius.val,      dx::nan<real>());
    external <real>        ("radius-tl",        m.radius.tl,       dx::nan<real>());
    external <real>        ("radius-tr",        m.radius.tr,       dx::nan<real>());
    external <real>        ("radius-bl",        m.radius.bl,       dx::nan<real>());
    external <real>        ("radius-br",        m.radius.br,       dx::nan<real>());
    /// --------------------------------------------------------------------------
    internal <bool>        ("captured",         m.captured,        false); /// cannot style these internals, they are used within style as read-only 'state'
    internal <bool>        ("focused",          m.focused,         false); /// so to add syntax to css you just add internals, boolean and other ops should be supported
    /// --------------------------------------------------------------------------
    internal <vec2>        ("cursor",           m.cursor,          vec2(null));
    internal <bool>        ("hover",            m.hover,           false);
    internal <bool>        ("active",           m.active,          false);
    ///
    m.region.style.manual_transition(true);
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

/*
void draw(Canvas &canvas) {
    canvas.color(m.fill.color);
    canvas.fill(paths.fill);
    canvas.color(m.text.color);
    canvas.scale(1.0);
    canvas.text(node::m.text.label, paths.border, node::m.text.align, { 0, 0 }, true);
}
*/

void node::draw(Canvas &canvas) {
    canvas.save();
    canvas.cap(m.border.cap());
    canvas.join(m.border.join());

    if (m.fill.color()) {
        canvas.color(m.fill.color());
        // case: class_name:Button, frame 1, should be fill-radius:16
        // also ideally we shouldnt crash if we have no radius
        canvas.fill(paths.fill); // style not loaded on first frame, looks like.
    }
    // image-multiply: #ffff [a multiply color, but not like multiply overlay]

    auto &image = m.fill.image();
    if (image) {
        canvas.image(image, paths.fill.bounds(), m.fill.align, vec2(0,0));
    }
    ///
    if (m.text) {
        canvas.color(m.text.color);
        canvas.text(m.text.label, paths.border, m.text.align, { 0.0, 0.0 }, true);
    }
    ///
    if (m.border.color() && m.border.size() > 0.0) {
        canvas.color(m.border.color());
        canvas.stroke_sz(m.border.size());
        canvas.stroke(paths.border);
    }
    ///
    canvas.restore();
}

str node::format_text() {
    return m.text.label;
}

void delete_node(node *n) {
    delete n;
}

rectd node::calculate_rect(node *child) {
    auto &reg = child->m.region;
    if (reg.style.transitioning()) {
        root->flags      |= node::StyleAnimate;
        Region    &reg0   =  reg.style.value_start;
        Region    &reg1   = *reg.style.selected;
        rectd      rect0  = reg0(paths.rect);
        rectd      rect1  = reg1(paths.rect);
        real       p      = reg.style.transition_pos();
        real       i      = 1.0 - p;
        return (rect0 * i) + (rect1 * p);
    } else if (reg.style.selected) {
        reg.style.value = *reg.style.selected;
    }
    return child->m.region()(paths.rect);
}
