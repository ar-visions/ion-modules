#pragma once
#include <dx/dx.hpp>
#include <ux/ux.hpp>
#include <ux/region.hpp>
#include <vk/vk.hpp>
#include <ux/style.hpp>

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

struct node;
struct Composer;
struct Construct;
struct Member;

typedef map<std::string, node *>     State;
typedef array<str> &PropList;

void delete_node(node *n);

size_t                    Style_members_count(str &s);
array<ptr<struct StBlock>> &Style_members(str &s);
StPair                   *Style_pair(Member *member);

double                    StBlock_match(struct StBlock *, struct node *);
struct StPair*            StBlock_pair (struct StBlock *b, str  &s);
size_t                    StPair_instances_count(struct StPair *p, Type &t);

struct Style;


struct Bind {
    str id, to;
    Bind(str id, str to = null):id(id), to(to ? to : id) { }
    bool operator==(Bind &b) { return id == b.id; }
    bool operator!=(Bind &b) { return !operator==(b); }
};

struct  node;
typedef node * (*FnFactory)();

typedef array<Bind> Binds;

struct Element {
    FnFactory         factory   = null;
    Binds             binds;
    array<Element>    elements;
    node             *ref       = 0;
    std::string       idc       = "";
    ///
    std::string &id() {
        if (idc.length())
            return idc;
        str id;
        for (auto &bind: binds)
            if (bind.id == "id") {
                id       = bind.to;
                break;
            }
        char buf[256];
        if (!id)
            sprintf(buf, "%p", (void *)factory); /// type-based token, effectively
        idc = id ? std::string(id) : std::string(buf);
        return idc;
    }
    Element(node *ref) : ref(ref) { }
    Element(std::nullptr_t n = null) { }
    Element(FnFactory factory, Binds &binds, array<Element> &elements):
        factory(factory), binds(binds), elements(elements) { }
    bool operator==(Element &b);
    bool operator!=(Element &b);
    operator bool()  { return ref || factory;     }
    bool operator!() { return !(operator bool()); }
    
    template <typename T>
    static Element each(array<T> &i, std::function<Element(T &v)> fn);
    
    template <typename K, typename V>
    static Element each(map<K, V> &m, std::function<Element(K &k, V &v)> fn);
};

typedef std::function<Element(void)> FnRender;

struct node {
    enum Flags {
        Focused      = 1,
        Captured     = 2,
        StateUpdate  = 4,
        StyleAnimate = 8
    };
    ///
    cchar_t         *selector   = "";
    cchar_t         *class_name = "";
    node            *parent     = null;
    map<std::string, node *> mounts;
    array<Element>   elements; /// elements cache
    Element          element;  ///
    std::queue<Fn>   queue;
    ///
    struct Paths {
        real       last_sg; /// identity for the last region-based rect update
        rectd      rect;
        Path       fill;
        Path       border;
        Path       child;
    } paths;
    ///
    bool           mounted    = false;
    node          *root       = nullptr;
    vec2           scroll     = {0,0};
    Binds          binds;
    int32_t        flags;
    map<std::string, PipelineData> objects;
    Style         *style = null;
    rectd          image_rect, text_rect;
    ///
    map<str, Member *> stationaries; /// statics, protected, idents, models, descriptors...
                                     /// we're going with stationary because everyone has loads of it they need to use for something.
    map<str, Member *> internals;
    map<str, Member *> externals;
    
    /// member type-specific struct
    template <typename T, const Member::MType MT>
    struct NMember:Member { /// add something. (from Member)
        struct StyleValue {
            public:
            str name;
                T            *selected     = null;
                StTrans      *trans        = null;
                bool          manual_trans = false;
                int64_t       timer_start  = 0;
                T             value_start  = T();
                T             value        = T();
            ///
            bool transitioning() const {
                return (!trans || trans->type == StTrans::None) ? false :
                        (timer_start + trans->duration.millis) > ticks();
            }
            ///
            real transition_pos() const {
                return (!trans || trans->type == StTrans::None) ? 1.0 :
                         std::clamp((ticks() - timer_start) / real(trans->duration.millis), real(0.0), real(1.0));
            }
            ///
            void manual_transition(bool manual) { manual_trans = manual; }
            ///
            void auto_update() {
                if (transitioning() && !manual_trans)
                    value = (*trans)(value_start, *selected, transition_pos());
            }
            ///
            inline void changed(T &snapshot, T *style_selection, StTrans *t) {
                if (t != trans) {
                    value_start = snapshot;
                    timer_start = ticks();
                    trans       = t;
                }
                selected = style_selection;
            }
            operator bool() { return selected != null; }
            T &operator()(T &def) {
                return transitioning() ? value : (selected ? *selected : def);
            }
            
        };
        size_t        cache = 0;
        T             def;
        T             value;
        StyleValue    style; // .value, .selected
        
        /// state cant and wont use style, not unless we want to create a feedback in spacetime. great. scott.
        bool state_bool() {
            T &v = cache ? value : def;
            if constexpr (std::is_same_v<T, std::filesystem::path>)
                return std::filesystem::exists(v);
            else
                return bool(v);
        }
        
        /// current effective value
        virtual T &current() {
            style.name = name;
            style.auto_update();
            return (cache != 0) ? value : style(def);
        }
        
        /// pointer to current effective value
        void *ptr() { return &current(); }
        
        /// called upon style selection with value of type T in the void *ptr
        /// if its in transition then we are animating
        void style_value_set(void *ptr, StTrans *t) {
            T &cur = current();
            style.changed(cur, (ptr ? (T *)ptr : (T *)null), t);
            if (style.transitioning())
                node->root->flags |= node::StyleAnimate;
        }
        
        void init() {
            type = Id<T>();
            static std::unordered_map<Type, Lambdas> lambdas;

            /// release the lambdas of war
            if (lambdas.count(type) == 0) {
                lambdas[type]       = {};
                
                /// unset value, use default value
                lambdas[type].unset = [](Member &m) -> void {
                    NMember<T,MT> &mm = (NMember<T,MT> &)m;
                    mm.cache        = 0;
                };
                
                /// boolean check, used with style. members extend the syntax of style. focus is just a standard member
                lambdas[type].get_bool = [](Member &m) -> bool {
                    NMember<T,MT> &mm = (NMember<T,MT> &)m;
                    if constexpr (std::is_same_v<T, std::filesystem::path>) {
                        T &v = (mm.cache != 0) ? mm.value : mm.style(mm.def);
                        return std::filesystem::exists(v);
                    } else
                        return bool(mm.current());
                };
                
                /// recompute style on this node:member
                if constexpr (MT == Extern)
                    lambdas[type].compute_style = [](Member &m) -> void {
                        StPair *p = Style_members_count(m.name) ? Style_pair(&m) : null;
                        NMember<T,MT> &mm = (NMember<T,MT> &)m;
                        ///
                        if (p && p->instances.count(m.type) == 0) {
                            if constexpr (is_vec<T>()) {
                                var conv = p->value;
                                p->instances.set(m.type, T::new_import(conv));
                            } else {
                                if constexpr (is_strable<T>() || std::is_base_of<EnumData, T>())                 /// this should be enforced.
                                    p->instances.set(m.type, new T(p->value));
                                else {
                                    var conv = p->value; /// var-based conversion is always an option
                                    p->instances.set(m.type, new T(conv));
                                }
                            }
                        }
                        mm.style_value_set(p ? p->instances.get<T>(m.type) : null, p ? &p->trans : null);
                    };
                else
                    lambdas[type].compute_style = null;
                
                /// direct set
                lambdas[type].type_set = [](Member &m, void *pv) -> void {
                    NMember<T,MT> &mm = (NMember<T,MT> &)m;
                    T *v = (T *)pv;
                    if constexpr (is_func<T>()) {
                        if (fn_id(mm.value) != fn_id(*v)) {
                            mm.value  = *v;
                            mm.cache++;
                        }
                    } else {
                        mm.value = *v;
                        mm.cache++;
                    }
                };

                if constexpr (std::is_base_of<io, T>() || std::is_same_v<T, path_t>) {
                    /// io conversion
                    if constexpr (!std::is_same_v<T, path_t>)
                        lambdas[type].var_get = [](Member &m) -> var { return var(((NMember<T,MT> &)m).current()); };
                    ///
                    lambdas[type].var_set = [&](Member &m, var &v) -> void {
                        NMember<T,MT> &mm = (NMember<T,MT> &)m;
                        if constexpr (is_vec<T>()) {
                            size_t sz = v.size();
                            T    conv = T(sz);
                            for (auto &i: v.a)
                                conv += static_cast<typename T::value_type>(i); /// static_cast not required, i think.  thats why its here.
                            mm.value  = conv;
                            mm.cache++;
                        } else if constexpr (is_func<T>()) {
                            T conv = T(v);
                            if (fn_id(mm.value) != fn_id(conv)) { /// todo: integrate into var
                                mm.value  = v;
                                mm.cache++;
                            }
                        } else {
                            T conv = T(v);
                            if (!(mm.value == conv)) {
                                mm.value = conv;
                                mm.cache++;
                            }
                        }
                    };
                }
            }
            /// lambdas for io conversion and direct are all type-mapped
            fn = &lambdas[type];
        }
        ///
        NMember() {
            init();
        }
        ///
        NMember(T v) : cache(1), value(v) {
            type = MT;
            init();
        }
        ///
        T &operator=(T v) const {
            assert(fn->type_set != null);
            fn->type_set(*this, (void *)&v);
            return value;
        }
        ///
        T &operator=(var  &v) {
            assert(fn->var_set != null);
            fn->var_set(*this, v);
            return value;
        }
        ///
        T &ref()           { return current(); }
           operator T&  () { return ref(); }
        T &operator()   () { return ref(); }
           operator bool() { return bool(ref()); }
    };           ///
    
    ///
    template <typename T>
    struct Stationary:NMember<T, Member::Stationary> {
        inline void operator=(T v) {
            assert(Member::fn->type_set != null);
            Member::fn->type_set(*this, (void *)&v);
        }
    };
    
    ///
    template <typename T>
    struct Intern:NMember<T, Member::Intern> {
        typedef T value_type;
        inline void operator=(T v) { Member::fn->type_set(*this, (void *)&v); }
    };
    
    ///
    template <typename T, typename C>
    struct Lambda:Intern<T> {
        typedef T value_type;
        std::function<T(C &)> lambda;
        ///
        T &current() { return (this->value = lambda(*(C *)Intern<T>::node)); }
    };
    
    ///
    template <typename T>
    struct Extern:NMember<T, Member::Extern> {
        typedef T value_type;
        inline void operator=(T v) { Member::fn->type_set(*this, (void *)&v); }
    };
    
    struct Border {
        Extern<double>  size;
        Extern<rgba>    color;
        Extern<bool>    dash;
        Extern<real>    offset;
        Extern<Cap>     cap;
        Extern<Join>    join;
        operator bool() { return size() > 0 && color().a > 0; }
    };
    
    struct Text {
        Extern<str>     label;
        Extern<rgba>    color;
        Extern<Region>  region; /// these three should be good. (region, align, offset)
        Extern<VAlign>  align;
        Extern<real>    offset;
        operator bool() { return label() && color().a > 0; }
    };
    
    struct Fill {
        Extern<rgba>    color;
        Extern<real>    offset;
        Extern<Image>   image;
        Extern<Region>  image_region;
        Extern<VAlign>  align; // repeat, rotate, scale, translate
        operator bool() { return offset() > 0 && color().a > 0; }
    };
    
    /// staaaaaation.
    template <typename T>
    void stationary(str name, Stationary<T> &nmem, T def) {
        nmem.type          =  Id<T>();
        nmem.member        =  Member::Stationary;
        nmem.name          =  name;
        nmem.def           =  def;
        stationaries[name] = &nmem;
    }
    
    template <typename T>
    void internal(str name, Intern<T> &nmem, T def) {
        nmem.type          =  Id<T>();
        nmem.member        =  Member::Intern;
        nmem.name          =  name;
        nmem.def           =  def;
        internals[name]    = &nmem;
    }
    
    template <typename T, typename C>
    void lambda(str name, Lambda<T,C> &nmem, std::function<T(C &)> fn) {
        nmem.type          =  Id<T>();
        nmem.member        =  Member::Intern;
        nmem.name          =  name;
        nmem.lambda        =  std::function<T(C &)>(fn);
        internals[name]    = &nmem;
    }
    
    template <typename T>
    void external(str name, Extern<T> &nmem, T def) {
        nmem.type          =  Id<T>();
        nmem.member        =  Member::Extern;
        nmem.name          =  name;
        nmem.def           =  def;
        externals[name]    = &nmem;
    }
    
    vec2 offset() {
        node *n = this;
        vec2  o = { 0, 0 };
        while (n) {
            rectd &rect = n->paths.rect;
            o  -= rect.xy();
            o  += n->scroll;
            n   = n->parent;
        }
        return o;
    }
    
    /// override intern (value)
    template <typename T>
    void override(Intern<T> &m, T v) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.def             = v;
        internals[m.name] = &m;
    }
    
    /// override extern (default value)
    template <typename T>
    void override(Extern<T> &m, T def) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.def             = def;
        externals[m.name] = &m;
    }
    
    struct Members {
        Stationary<str> id;
        Extern<str>    bind;
        Extern<int>    tab_index;
        Text           text;
        Fill           fill;
        Border         border;
        Fill           child;
        Extern<Region> region;
        ///
        struct Radius {
            enum Side { TL, TR, BR, BL };
            Extern<real> val;
            Extern<real> tl;
            Extern<real> tr;
            Extern<real> br;
            Extern<real> bl;
            operator bool() { return val() > 0 || tl() > 0 || tr() > 0 || br() > 0 || bl() > 0; }
            real operator()(Side side) {
                real v = std::isnan(val()) ? 0 : val();
                switch (side) {
                    case TL: return std::isnan(tl()) ? v : tl();
                    case TR: return std::isnan(tr()) ? v : tr();
                    case BL: return std::isnan(bl()) ? v : bl();
                    case BR: return std::isnan(br()) ? v : br();
                }
                return 0;
            }
        } radius;
        ///
        Intern<bool>   captured;
        Intern<bool>   focused;
        Intern<vec2>   cursor;
        Intern<bool>   active;
        Intern<bool>   hover;
        ///
        struct Events {
            Extern<Fn> hover;
            Extern<Fn> out;
            Extern<Fn> down;
            Extern<Fn> up;
            Extern<Fn> key;
            Extern<Fn> focus;
            Extern<Fn> blur;
            Extern<Fn> cursor;
        } ev;
    } m;
                    node(std::nullptr_t n = null);
                    node(cchar_t *selector, cchar_t *cn, Binds binds, array<Element> elements);
    virtual        ~node();
            void    standard_bind();
            void    standard_style();
    virtual void    bind();
    virtual void    mount();
    virtual void    umount();
    virtual void    changed(PropList props);
    virtual void    input(str k);
    virtual Element render();
    virtual void    draw(Canvas &canvas);
    virtual str     format_text();
    bool            processing();
    bool            process();
    node           *find(std::string n);
    node           *select_first(std::function<node *(node *)> fn);
    array<node *>     select(std::function<node *(node *)> fn);
    void            exec(std::function<void(node *)> fn);
    void            focus();
    void            blur();
    node           *focused();
    virtual rectd   region_rect(int index, Region &reg, rectd &src, node *n);
    
    /// region to rect calculation done here for nodes
    struct RegionOp {
        node::Extern<Region> &reg;
        node           *rel;
        rectd          &rect;
        rectd          &result;
        void operator()(node *n, int index) {
            auto root = n->root ? n->root : n;
            if (reg.style.transitioning()) {
                root->flags      |= node::StyleAnimate;
                rectd      rect0  =  rel->region_rect(index,  reg.style.value_start, rect, n);
                rectd      rect1  =  rel->region_rect(index, *reg.style.selected,    rect, n);
                real       p      =  reg.style.transition_pos();
                result            = (rect0 * (1.0 - p)) + (rect1 * p);
            } else
                result            = rel->region_rect(index, reg, rect, n);
        }
    };
    
    template <typename T>
    inline T state(str name) {
        if constexpr (std::is_same_v<T, bool>) {
            if (externals.count(name))
                return externals[name]->state_bool();
            if (internals.count(name))
                return internals[name]->state_bool();
        }
        return T(null);
    }
    
    Device &device() { return Vulkan::device(); }
    
    template <typename U>
    UniformData uniform(int id, std::function<void(U &)> fn) { return UniformBuffer<U>(device(), id, fn); }
    
    template <typename T>
    VertexData vertices(array<T> &v_vbo) { return VertexBuffer<Vertex>(device(), v_vbo); }
    
    template <typename I>
    IndexData polygons(array<I>  &v_ibo) { return IndexBuffer<I>(device(), v_ibo); }
    
    template <typename V>
    Pipes model(path_t path, UniformData  ubo, VAttr attr, Shaders shaders = null) {
        return Model<V>(device(), ubo, attr, path);
    }
    
    Pipeline<Vertex> texture(Texture tx, UniformData ubo) {
        auto v_sqr = Vertex::square();
        auto i_sqr = array<uint32_t> { 0, 1, 2, 2, 3, 0 };
        auto   vbo = vertices(v_sqr);
        auto   ibo = polygons(i_sqr);
        return Pipeline<Vertex> {
            device(), ubo, vbo, ibo,
            { Position3f(), Normal3f(), Texture2f(tx), Color4f() },
              rgba {1.0, 0.7, 0.2, 1.0}, "main" };
    }
    
    inline size_t count(std::string n) {
        for (auto &e: elements)
            for (auto &bind: e.binds)
                if (bind.id == n)
                    return 1;
        return 0;
    }
    
    template <typename T>
    T &context(str name) {
        static T t_null = null;
        node  *n = this;
        while (n)
            if (n->stationaries.count(name))
                return ((Intern<T> *)n->stationaries[name])->value;
            else
                n = n->parent;
        return t_null;
    }
};

#define declare(T)\
    T(Binds binds = {}, array<Element> elements = {}):\
        node((cchar_t *)"", (cchar_t *)TO_STRING(T), binds, elements) { }\
        inline static T *factory() { return new T(); };\
        inline operator  Element() { return { FnFactory(T::factory), node::binds, node::elements }; }

struct Group:node {
    declare(Group);
};
