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
typedef vec<str> &PropList;

void delete_node(node *n);

typedef std::function<void(Member &, var &)>  MFn;
typedef std::function< var(Member &)>         MFnGet;
typedef std::function<bool(Member &)>         MFnBool;
typedef std::function<void(Member &)>         MFnVoid;
typedef std::function<void(Member &, void *)> MFnArb;

struct Member {
    enum MType {
        Undefined,
        Context,
        Intern,
        Extern
    };
    ///
    Type     type       =  Type::Undefined;
    MType    member     = MType::Undefined;
    str      name       = null;
    size_t   cache      = 0;
    size_t   peer_cache = 0xffffffff;
    Member  *bound_to   = null;
    ///
    virtual void *ptr() { return null; }
    ///
    virtual bool state_bool()  {
        assert(false);
        return bool(false);
    }
    ///
    /// once per type (todo)
    struct Lambdas {
        MFn      var_set;
        MFnGet   var_get;
        MFnBool  get_bool;
        MFnVoid  compute_style;
        MFnArb   type_set;
        MFnVoid  unset;
    } *fn = null;
    ///
    node    *node;
    ///
    void operator=(const str &s) {
        var v = s;
        if (fn && fn->var_set)
            fn->var_set(*this, v);
    }
    ///
    void operator=(Member &v);
    ///
    virtual bool operator==(Member &m) {
        return bound_to == &m && peer_cache == m.peer_cache;
    }
    
    virtual void style_value_set(void *ptr, StTrans *) { }
};

size_t Style_members_count(str &s);
vec<ptr<struct StBlock>> &Style_members(str &s);
double StBlock_match(struct StBlock *, struct node *);
struct StPair* StBlock_pair (struct StBlock *b, str  &s);
size_t StPair_instances_count(struct StPair *p, Type &t);
StPair *Style_pair(Member *member);

struct Style;

struct node {
    enum Flags {
        Focused      = 1,
        Captured     = 2,
        StateUpdate  = 4,
        StyleAnimate = 8
    };
    ///
    const char    *selector   = "";
    const char    *class_name = "";
    node          *parent     = null;
    map<std::string, node *> mounts;
    vec<Element>   elements; /// elements cache
    Element        element;  ///
    std::queue<Fn> queue;
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
    ///
    map<str, Member *> contextuals;
    map<str, Member *> internals;
    map<str, Member *> externals;
    
    /// member type-specific struct
    template <typename T, const Member::MType MT>
    struct MType:Member {
        struct StyleValue {
           
            ///
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
            /// text-label has style_selection "legit str" and snapshot is some farse.
            /// the Type on StTrans is random data, otherwise seems to be initialized correctly
            ///
            inline void changed(T &snapshot, T *style_selection, StTrans *t) {
                if (name == "text-label") {
                    int test = 0;
                    test++;
                }
                if (t != trans) {
                    value_start = snapshot; /// why is this invalid?
                    timer_start = ticks();
                    trans       = t;
                }
                selected = style_selection;
                if (name == "text-label") {
                    int test = 0;
                    test++;
                }
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
        
        /// state cant and wont use style, not unless we want to create a feedback in spacetime
        /// great. scott.
        bool state_bool() {
            T &v = cache ? value : def;
            if constexpr (std::is_same_v<T, std::filesystem::path>)
                return std::filesystem::exists(v);
            else
                return bool(v);
        }
        
        /// current effective value
        T &current() {
            style.name = name;
            style.auto_update();
            return (cache != 0) ? value : style(def);
        }
        
        /// pointer to current effective value
        void *ptr() { return &current(); }
        
        /// extern, intern, context and such could override; only to disable most likely
        /// perhaps you may want intern to use the style only if its default is null
        void style_value_set(void *ptr, StTrans *t) {
            if (name == "text-label") { // right when the test code goes in, it starts to work. maybe no one will notice.
                int test = 0;
                test++;
            }
            T &cur = current();
            if (name == "text-label") {
                int test = 0;
                test++;
            }
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
                    MType<T,MT> &mm = (MType<T,MT> &)m;
                    mm.cache        = 0;
                };
                
                /// boolean check, used with style. members extend the syntax of style. focus is just a standard member
                lambdas[type].get_bool = [](Member &m) -> bool {
                    MType<T,MT> &mm = (MType<T,MT> &)m;
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
                        MType<T,MT> &mm = (MType<T,MT> &)m;
                        ///
                        if (p && p->instances.count(m.type) == 0) {
                            if constexpr (is_vec<T>()) {
                                var conv = p->value;
                                p->instances.set(m.type, T::new_import(conv));
                            } else {
                                if constexpr (is_strable<T>())
                                    p->instances.set(m.type, new T(p->value));
                                else {
                                    var conv = p->value;
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
                    MType<T,MT> &mm = (MType<T,MT> &)m;
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
                
                /// io conversion
                if constexpr (std::is_base_of<io, T>() || std::is_same_v<T, path_t>) {
                    if constexpr (!std::is_same_v<T, path_t>)
                        lambdas[type].var_get = [](Member &m) -> var { return var(((MType<T,MT> &)m).current()); };
                    ///
                    lambdas[type].var_set = [&](Member &m, var &v) -> void {
                        MType<T,MT> &mm = (MType<T,MT> &)m;
                        if constexpr (is_vec<T>()) {
                            size_t sz = v.size();
                            T    conv = T(sz);
                            for (auto &i: *v.a)
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
        MType() {
            init();
        }
        ///
        MType(T v) : cache(1), value(v) {
            type = MT;
            init();
        }
        ///
        ///
        T &operator=(T v) const {
            fn->type_set(*this, (void *)&v);
            return value;
        }
        T &operator=(var  &v) {
            fn->var_set(*this, v);
            return value;
        }
        ///
        T &ref()           { return current(); }
           operator T&  () { return ref(); }
        T &operator()   () { return ref(); }
           operator bool() { return bool(ref()); }
    };           ///
    
    template <typename T>
    struct Context:MType<T, Member::Context> {
        inline void operator=(T v) {
            Member::fn->type_set(*this, (void *)&v);
        }
    };
    
    template <typename T>
    struct Intern:MType<T, Member::Intern> {
        typedef T value_type;
        inline void operator=(T v) {
            Member::fn->type_set(*this, (void *)&v);
        }
    };
    
    template <typename T>
    struct Extern:MType<T, Member::Extern> {
        typedef T value_type;
        inline void operator=(T v) {
            Member::fn->type_set(*this, (void *)&v);
        }
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
        Extern<AlignV2> align;
        Extern<real>    offset;
        operator bool() { return label() && color().a > 0; }
    };
    
    struct Fill {
        Extern<rgba>    color;
        Extern<real>    offset;
        Extern<Image>   image;
        Extern<AlignV2> align; // repeat, rotate, scale, translate
        operator bool() { return offset() > 0 && color().a > 0; }
    };
    
    template <typename T>
    void contextual(str name, Context<T> &m, T def) {
        m.type          = Id<T>();
        m.member        = Member::Context;
        m.name          = name;
        m.def           = def;
        contextuals[name] = &m;
    }
    
    template <typename T>
    void internal(str name, Intern<T> &m, T def) {
        m.type          = Id<T>();
        m.member        = Member::Intern;
        m.name          = name;
        m.def           = def;
        internals[name] = &m;
    }
    
    template <typename T>
    void external(str name, Extern<T> &m, T def) {
        m.type          = Id<T>();
        m.member        = Member::Extern;
        m.name          = name;
        m.def           = def;
        externals[name] = &m;
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
    
    ///
    template <typename T>
    void override(Intern<T> &m, T def) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.def             = def;
        internals[m.name] = &m;
    }
    
    template <typename T>
    void override(Extern<T> &m, T def) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.def             = def;
        externals[m.name] = &m;
    }
    
    struct Members {
        Context<str>   id;
        Extern<str>    bind;
        Extern<int>    tab_index;
        Text           text;
        Fill           fill;
        Border         border;
        Fill           child;
        Extern<Region> region;
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
                    node(nullptr_t n = null);
                    node(const char *selector, const char *cn, Binds binds, vec<Element> elements);
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
    vec<node *>     select(std::function<node *(node *)> fn);
    void            exec(std::function<void(node *)> fn);
    virtual rectd   calculate_rect(node *child);
    void            focus();
    void            blur();
    node           *focused();
    
    int             u_next_id = 0;
    
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
    
    Device &device() {
        return Vulkan::device();
    }
    
    template <typename U>
    UniformData uniform(int id, std::function<void(U &)> fn) {
        return UniformBuffer<U>(device(), id, fn);
    }
    
    template <typename T>
    VertexData vertices(vec<T> &v_vbo) {
        return VertexBuffer<Vertex>(device(), v_vbo);
    }
    
    template <typename I>
    IndexData polygons(vec<I> &v_ibo) {
        return IndexBuffer<I>(device(), v_ibo);
    }
    
    template <typename V>
    PipelineMap model(path_t       path, UniformData  ubo,
                      vec<Attrib>  attr, Shaders      shaders = null) {
        return Model<V>(device(), ubo, attr, path);
    }
    
    /// vice versa pattern with above ^
    /// simple texture output with ubo controller
    Pipeline<Vertex> texture(Texture tx, UniformData ubo) {
        auto v_sqr = Vertex::square();
        auto i_sqr = vec<uint32_t> { 0, 1, 2, 2, 3, 0 };
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
            if (n->contextuals.count(name))
                return ((Intern<T> *)n->contextuals[name])->value;
            else
                n = n->parent;
        return t_null;
    }
};

#define declare(T)\
    T(Binds binds = {}, vec<Element> elements = {}):\
        node((const char *)"", (const char *)TO_STRING(T), binds, elements) { }\
        inline static T *factory() { return new T(); };\
        inline operator  Element() { return { FnFactory(T::factory), node::binds, node::elements }; }

struct Group:node {
    declare(Group);
};

/// not much more amazement fits within 575 lines. this line is outside of that count. it is not amazing.
