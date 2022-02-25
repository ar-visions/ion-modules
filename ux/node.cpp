#include <ux/ux.hpp>

// fn_type_set not called via update, and this specific call is only when its assigned by binding
void Member::operator=(Member &v) {
    if (type == v.type)
        lambdas->type_set(*this, v.shared); // the 0-base never free anything.
    else {
        assert(v.lambdas->var_get); /// must inherit from io
        assert(v.lambdas->var_set);
        var val = v.lambdas->var_get(*this);
        lambdas->var_set(*this, val); //
    }
    bound_to   = &v;
    peer_cache = v.cache; /// needs to be updated in type_set()
}

std::unordered_map<Type, Member::Lambdas> Member::lambdas_map;

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

array<node *> node::select(std::function<node *(node *)> fn) {
    array<node *> result;
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

node::node(cchar_t *class_name, str id, Binds binds, Elements elements):
    id(id), class_name(class_name), binds(binds), elements(elements) { }

Element::Element(FnFactory factory, str id, Binds binds, Elements elements)
        :e(std::shared_ptr<ElementData>(new ElementData(null))) {
    e->id       = id;
    e->elements = elements;
    e->binds    = binds;
    e->factory  = factory;
}

///
str &ElementData::ident() {
    if (id)
        return id;
    if (ident_cache)
        return ident_cache;
    char buf[256];
    sprintf(buf, "%p", (void *)factory); /// type-based token, effectively
    ident_cache = str(buf);
    return ident_cache;
}

node::node(std::nullptr_t n) { }

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
                //n->queue.front()(zero);
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
  //stationary<str>        ("id",               m.id,              str(""));
    /// --------------------------------------------------------------------------
    external <int>         ("tab-index",        m.tab_index,       -1);
    /// --------------------------------------------------------------------------
    external <str>         ("text-label",       m.text.label,      str(""));
    external <VAlign>      ("text-align",       m.text.align,      VAlign { Align::Middle, Align::Middle });
    external <rgba>        ("text-color",       m.text.color,      rgba("#fff") );
    external <real>        ("text-offset",      m.text.offset,     real(0));
    external <Region>      ("text-region",      m.text.region,     null);
    /// --------------------------------------------------------------------------
    external <rgba>        ("fill-color",       m.fill.color,      rgba(0,255,0,0));
    external <real>        ("fill-offset",      m.fill.offset,     real(0));
    external <VAlign>      ("image-align",      m.fill.align,      VAlign(null));
    external <Image>       ("image",            m.fill.image,        null);
    external <Region>      ("image-region",     m.fill.image_region, null);
    external <double>      ("border-size",      m.border.size,     double(0));
    external <rgba>        ("border-color",     m.border.color,    rgba("#000f"));
    external <bool>        ("border-dash",      m.border.dash,     bool(false));
    external <real>        ("border-offset",    m.border.offset,   real(0));
    external <real>        ("child-offset",     m.child.offset,    real(0));
    external <real>        ("fill-offset",      m.fill.offset,     real(0));
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
    external <Fn>          ("ev-cursor",        m.ev.cursor,       null);
    /// --------------------------------------------------------------------------
    external <real>        ("radius",           m.radius.val,      dx::nan<real>());
    external <real>        ("radius-tl",        m.radius.tl,       dx::nan<real>());
    external <real>        ("radius-tr",        m.radius.tr,       dx::nan<real>());
    external <real>        ("radius-bl",        m.radius.bl,       dx::nan<real>());
    external <real>        ("radius-br",        m.radius.br,       dx::nan<real>());
    /// --------------------------------------------------------------------------
    internal <bool>        ("captured",         m.captured,        false);
    internal <bool>        ("focused",          m.focused,         false); /// internals cannot be set in style but they can be selected by boolean or value op
    /// --------------------------------------------------------------------------
    internal <vec2>        ("cursor",           m.cursor,          vec2(null));
    internal <bool>        ("hover",            m.hover,           false);
    internal <bool>        ("active",           m.active,          false);

    ///
    m.region.style.manual_transition(true);
    m.fill.image_region.style.manual_transition(true);
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
    if (m.fill.color()) { /// free'd prematurely during style change (not a transition)
        canvas.color(m.fill.color());
        canvas.fill(paths.fill);
    }
    ///
    auto &image = m.fill.image();
    if (image)
        canvas.image(image, image_rect, (vec2 &)m.fill.align(), vec2(0,0));
    
    ///
    if (m.text) {
        canvas.color(m.text.color);
        canvas.text(m.text.label, paths.border, (vec2 &)m.text.align(), { 0.0, 0.0 }, true);
    }
    ///
    if (m.border.color() && m.border.size() > 0.0) {
        canvas.color(m.border.color());
        canvas.outline_sz(m.border.size());
        canvas.outline(paths.border);
    }
}

str node::format_text() {
    return m.text.label;
}

void delete_node(node *n) {
    delete n;
}

rectd node::region_rect(int index, Region &reg, rectd &src, node *n) {
    return reg(src);
}
