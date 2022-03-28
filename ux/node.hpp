#pragma once
#include <dx/dx.hpp>
#include <ux/ux.hpp>
#include <ux/region.hpp>
#include <vk/vk.hpp>
#include <ux/style.hpp>

struct node;
struct Composer;
struct Construct;
struct Member;

typedef map<std::string, node *> State;
typedef array<str> &PropList;

void delete_node(node *n);

size_t                      Style_members_count(str &s);
array<ptr<struct StBlock>> &Style_members(str &s);
StPair                     *Style_pair(Member *member);
///
double                      StBlock_match(struct StBlock *, struct node *);
struct StPair*              StBlock_pair (struct StBlock *b, str  &s);
size_t                      StPair_instances_count(struct StPair *p, Type &t);

struct Style;
struct Element;

///
struct Arg {
    Shared shared;
    
    /// constructor selection needs disambiguation and clarity in C++, i think silver could do a better job with less syntax and more usability
    template <typename T>
    Arg(T v) {
        if constexpr (std::is_base_of<Shared, T>())
            shared = v;
        else if constexpr (std::is_same_v<const char *, T> || std::is_same_v<char *, T>)
            shared = Managed<str> { new str((const char *)v) };
        else
            shared = Managed<T>   { new T(v) };
    }
    
    Arg(nullptr_t n = null) { }
};

///
struct Bind {
    str       id;
    Shared    shared;
    
    Bind(str id, Arg a = null) : id(id), shared(a.shared) {
        int test = 0;
        test++;
    }
    Bind(Arg a)         :         shared(a.shared) { }
    
    bool operator==(Bind &b) { return id == b.id; }
    bool operator!=(Bind &b) { return !operator==(b); }
};

/// something not possible with array<Bind> [/kicks rock]
/*
struct Binds {
    std::vector<Bind> a;
    static typename std::vector<Bind>::iterator iterator;

    Bind                &back()              { return a.back();       }
    Bind               &front()              { return a.front();      }
    Size                 size()        const { return a.size();       }
    Size             capacity()        const { return a.capacity();   }
    typeof(iterator)    begin()              { return a.begin();      }
    typeof(iterator)      end()              { return a.end();        }
              operator   bool()        const { return a.size() > 0;   }
    bool      operator      !()        const { return !(operator bool()); }
    Bind&     operator     [](size_t i)      { return a[i];           }
    bool operator== (Binds &b)               {
        bool equal = a.size() == b.a.size();
        if  (equal) for (size_t i = 0; i < a.size(); i++)
            if (a[i] != b.a[i]) {
                equal = false;
                break;
            }
        return equal;
    }
    bool operator!= (Binds &b)               { return operator==(b);  }

    Binds() { }
    Binds(std::initializer_list<Bind> args) {
        size_t sz = args.size();
        a.reserve(sz);
        for (auto &i: args)
            a.push_back(i);
    }
};
*/

typedef array<Bind> Binds;

struct  node;
typedef node         * (*FnFactory)();
typedef array<Element>   Elements;
//typedef array<Bind>    Binds; /// not movable
///
struct  ElementData {
    FnFactory       factory   = null;
    str             id;
    str             ident_cache;
    Binds           binds;
    Elements        elements;
    node           *ref2      = 0;
    str            &ident();
    ///
    ElementData(std::nullptr_t n = null)  { }
    ElementData(node  *ref) : ref2(ref)   { }
    ElementData(size_t res) : elements(res) { }
};

struct Element {
    std::shared_ptr<ElementData>        e;
    Element(size_t          res)      : e(std::shared_ptr<ElementData>(new ElementData(res)))  { }
    Element(node           *ref)      : e(std::shared_ptr<ElementData>(new ElementData(ref)))  { }
    Element(std::nullptr_t  n = null) : e(std::shared_ptr<ElementData>(new ElementData(null))) { }
    Element(FnFactory factory, str id, Binds binds, Elements el);

    ///
    node          *ref() const { return e->ref2; }
    node      *factory() const { return e->factory ? e->factory() : null; }
    operator      bool() const { return e->ref2   || e->factory || e->elements; }
    bool     operator!() const { return !operator bool(); }
    str         &ident()       { return e->ident(); }
    Elements &elements() const { return e->elements; }
    Binds       &binds() const { return e->binds; }
    bool constructable() const { return e->factory != null; }
    Size          size() const { return e->elements.size(); }
    
    ///
    bool operator!=(Element &eb) const { return !operator==(eb); }
    bool operator==(Element &eb) const {
        ElementData &a =    *e;
        ElementData &b = *eb.e;
        return a.factory  == b.factory &&
               a.binds    == b.binds   &&
               a.elements == b.elements;
    }

    /// heres a use-case for the nullable type, return null with this function
    template <typename T>
    static Element filter(array<T> a, std::function<Element(T &v)> fn) {
        Element e(a.size());
        for (auto &v:a) {
            Element r = fn(v);
            if (r)
                e.elements() += r;
        }
        return e;
    }
    
    template <typename K, typename V>
    static Element mapz(map<K, V> m, std::function<Element(K &k, V &v)> fn) {
        Element e(m.size);
        for (auto &[k,v]:m) {
            Element r = fn(k,v);
            if (r)
                e.elements() += r;
        }
        return e;
    }
};

typedef std::function<Element(void)> FnRender;

/// purpose-built for multi-domain use component model.
/// your business logic encapsulated in isolated design,
/// a sort of lexical type lookup (closest in scope)
/// for type-strict context usage-case.
struct node {
    enum Flags {
        Focused      = 1,
        Captured     = 2,
        StateUpdate  = 4,
        StyleAnimate = 8,
        
    };
    
    ///
    str                id;
    map<std::string, node *> mounts;
    cchar_t           *class_name = "";
    node              *parent     = null;
    bool               mounted    = false;
    node              *root       = nullptr;
    vec2               scroll     = {0,0};
    Binds              binds;    ///
    Elements           elements; /// elements cache # skipping this, along with binds; we use both in element which is shared
    std::queue<Fn>     queue;    /// this is an animation queue
    int32_t            flags;
    map<std::string, PipelineData> objects;
    Style             *style = null;
    rectd              image_rect, text_rect;
    map<str, Member *> internals;
    map<str, Member *> externals;
    
    bool operator==(node &b) { return this == &b; }
    operator bool()          { return true; }
    
    virtual bool value_update(void *v_value) {
        assert(false);
        return false;
    }
    ///
    struct Paths {
        real       last_sg; /// identity for the last region-based rect update
        rectd      rect;
        Stroke     fill;
        Stroke     border;
        Stroke     child;
    } paths;
    
    /// member type-specific struct
    template <typename T, const Member::MType MT>
    struct NMember:Member {
        
        struct StyleValue {
            public:
            NMember<T, MT> *host       = null;
            StTrans      *trans        = null;
            bool          manual_trans = false;
            int64_t       timer_start  = 0;
            sh<T>         t_start;       // transition start value (could be in middle of prior transition, for example, whatever its start is)
            sh<T>         t_value;       // current transition value
            sh<T>         s_value;       // style value
            sh<T>         d_value; // default value
            
            ///
            inline T &current() { return (T &)(sh<T> &)host->shared_value(); }
            
            ///
            T &auto_update() {
                assert(host);
                if (!host->shared && transitioning() && !manual_trans)
                    *t_value = (*trans)(*t_start, *s_value, transition_pos());
                return current();
            }
            
            void manual_transition(bool manual) { manual_trans = manual; }
            bool transitioning()  const { return (!trans || trans->type == StTrans::None) ? false : (timer_start + trans->duration.millis) > ticks(); }
            real transition_pos() const { return (!trans || trans->type == StTrans::None) ? 1.0 :
                         std::clamp((ticks() - timer_start) / real(trans->duration.millis), real(0.0), real(1.0));
            }
            
            inline void changed(T &snapshot, T *s_value, StTrans *t) {
                if (t != trans) {
                    if (t) {
                        t_start = new T(snapshot);
                        if (!t_value)
                             t_value = new T(snapshot);
                        timer_start = ticks();
                    }
                    trans = t;
                }
                this->s_value = s_value ? new T(*s_value) : (T *)null;
            }
        };
        size_t        cache = 0;
        sh<T>         d_value;
        StyleValue    style;
        
        virtual Shared &shared_value() {
            return                   shared ? (Shared &)shared        : ///
                      style.transitioning() ? (Shared &)style.t_value : /// more freedom than the shared_ptr, and it has attachment potential
                     style.s_value.is_set() ? (Shared &)style.s_value : /// style value (transition-to parameter as well)
                                              (Shared &)      d_value ; /// default value (base value, if none above are set and not in transition)}
        }
        
        /// this cannot use style, because its used by style
        bool state_bool() {
            if (!shared.raw())
                return false;
            T v = *(T *)shared;
            if constexpr (std::is_same_v<T, std::filesystem::path>)
                return std::filesystem::exists(v);
            else if constexpr (can_convert<T, bool>::value)
                return bool(v);
            return false;
        }
        virtual T    &current() { return  style.auto_update(); }
        void             *ptr() { return &current();           }
        Shared     shared_ptr() { return  current();           }
    
        void style_value_set(void *ptr, StTrans *t) {
            T &cur = current();
            style.changed(cur, (ptr ? (T *)ptr : (T *)null), t);
            if (style.transitioning()) {
                assert(arg);
                ((node *)arg)->root->flags |= node::StyleAnimate;
            }
        }
        
        void integrate() {
            style.host    = this;
            style.d_value = d_value;
            
            /// release the lambdas of war
            if (Member::lambdas_map.count(type) == 0) {
                Member::lambdas_map[type]   = {};
                lambdas                     = &Member::lambdas_map[type];
                
                /// unset value, use default value
                lambdas->unset = [](Member &m) -> void {
                    NMember<T,MT> &mm = (NMember<T,MT> &)m;
                    mm.cache = 0;
                };
                
                /// members extend the syntax of style
                lambdas->get_bool = [](Member &m) -> bool {
                    NMember<T,MT> &mm = (NMember<T,MT> &)m;
                    return bool(mm.state_bool());
                };
                
                /// recompute style on this node:member
                if constexpr (MT == Extern)
                    lambdas->compute_style = [](Member &m) -> void {
                        StPair *p = Style_members_count(m.name) ? Style_pair(&m) : null;
                        NMember<T,MT> &mm = (NMember<T,MT> &)m;
                        ///
                        if (p && p->instances.count(m.type) == 0) {
                            if constexpr (is_vec<T>()) {
                                var conv = p->value;
                                p->instances.set(m.type, new T(conv));
                            } else if constexpr (is_map<T>()) {
                                assert(false);
                            } else if constexpr (is_strable<T>() || std::is_base_of<EnumData, T>())
                                p->instances.set(m.type, new T(p->value));
                              else {
                                // if type is primitive-based
                                if constexpr (Type::spec<T>() != Type::Undefined) {
                                    /// it could be configured to fault here
                                    var conv = p->value;
                                    p->instances.set(m.type, new T(conv));
                                } else {
                                    console.error("eject porkins");
                                }
                            }
                        }
                        /// merge instances, and var std::shared use-cases with Shared
                        mm.style_value_set(p ? p->instances.get<T>(m.type) : null, (p && p->trans) ? &p->trans : null);
                    };
                else
                    lambdas->compute_style = null;
                
                lambdas->type_set = [](Member &m, Shared &shared) -> void {
                    NMember<T,MT> &mm  =  (NMember<T,MT> &)m;
                    ///
                    if constexpr (is_func<T>()) {
                        /// nice technique shared
                        if (fn_id((T &)mm.shared) != fn_id((T &)shared)) {
                            mm.shared = shared;
                            mm.cache++;
                        }
                    } else if (mm.shared != shared) {
                        mm.shared = shared;
                        mm.cache++;
                    }
                    /// must flag for style update
                    /// this needs a general fix, then i wnat to get ux-app running fine, css watch needs to work, then i am going to render the uv globe, map
                    node *n_arg = mm.arg;
                    if (n_arg)
                        n_arg->flags |= node::StateUpdate;
                };

                if constexpr (std::is_base_of<io, T>() || std::is_same_v<T, path_t>) {
                    /// io conversion
                    if constexpr (!std::is_same_v<T, path_t>)
                        lambdas->var_get = [](Member &m) -> var { return var(((NMember<T,MT> &)m).current()); };
                    ///
                    lambdas->var_set = [&](Member &m, var &v) -> void {
                        NMember<T,MT> &mm = (NMember<T,MT> &)m;
                        if constexpr (is_vec<T>()) {
                            size_t sz = v.size();
                            T   *conv = new T(sz);
                            for (auto &i: v.a)
                                *conv += static_cast<typename T::value_type>(i); /// static_cast not required, i think.  thats why its here.
                            mm.shared = conv;
                            mm.cache++;
                        } else if constexpr (is_map<T>()) {
                            assert(false);
                        } else if constexpr (is_func<T>()) {
                            T *conv = new T(v);
                            if (fn_id(mm.value) != fn_id(*conv)) { /// todo: integrate into var
                                mm.shared = conv;
                                mm.cache++;
                            } else
                                delete conv;
                        } else {
                            mm.shared = new T(v);
                            mm.cache++;
                        }
                    };
                }
            } else {
                /// set reference to said warrior lambdas
                lambdas = &Member::lambdas_map[type];
            }
        }
            
        ///
        NMember() { }
        ///
        NMember(T v) : cache(0) {
            type = MT;
            shared = v;
        }
        ///
        T &operator=(T v) const {
            assert(lambdas->type_set != null);
            shared = v;
            lambdas->type_set(*this, shared);
            return shared;
        }
        ///
        T &operator=(var  &v) {
            assert(lambdas->var_set != null);
            lambdas->var_set(*this, v);
            return shared;
        }
        ///
        T &ref()           { return current(); }
           operator T&  () { return ref(); }
        T &operator()   () { return ref(); }
           operator bool() { return bool(ref()); }
    };           ///
    
    ///
    template <typename T>
    struct Intern:NMember<T, Member::Intern> {
        typedef T value_type;
        inline void operator=(T v) {
            Shared s = sh<T>(new T(v));
            Member::lambdas->type_set(*this, s);
        }
        operator Arg() {
            Shared s = NMember<T, Member::Intern>::shared_value();
            return Arg { s };
        }
    };
    
    ///
    template <typename T>
    struct Extern:NMember<T, Member::Extern> {
        typedef T value_type;
        inline void operator=(T v) {
            Shared s = sh<T>(new T(v));
            Member::lambdas->type_set(*this, s);
        }
        operator Arg() { return Arg { NMember<T, Member::Extern>::shared_value() }; }
    };
    
    ///
    template <typename T, typename C>
    struct Lambda:Extern<T> {
        typedef T value_type;
        std::function<T(C &)> lambda;
        ///
        Shared &shared_value() { return (Shared &)(Member::shared = new T(lambda(*(C *)Extern<T>::arg))); }
        T &current()           { return (T &)shared_value(); }
        operator Arg()         { return Arg { shared_value() }; }
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
    void internal(str name, Intern<T> &nmem, T def) {
        nmem.type          = Id<T>();
        nmem.member        = Member::Intern;
        nmem.name          = name;
        nmem.d_value       = Managed <T>  { new T(def) };
        nmem.arg           = Unsafe<node> { this };
        /// even with an unsafe ptr you can attach some stuff when the 'share pool' is no longer. it just doesnt free the dingy
        internals[name]    = &nmem;
        nmem.integrate();
    }
    
    template <typename R, typename T>
    void lambda(str name, Lambda<R,T> &nmem, std::function<R(T &)> fn) {
        nmem.type          = Id<R>();
        nmem.member        = Member::Extern;
        nmem.name          = name;
        nmem.lambda        = std::function<R(T &)>(fn);
        nmem.arg           = Unsafe <node> { this };
        externals[name]    = &nmem;
        nmem.integrate();
    }
    
    template <typename T>
    void external(str name, Extern<T> &nmem, T def) {
        nmem.type          = Id<T>();
        nmem.member        = Member::Extern;
        nmem.name          = name;
        nmem.d_value       = Managed <T>   { new T(def) };
        nmem.arg           = Unsafe <node> { this };
        externals[name]    = &nmem;
        nmem.integrate();
    }
    
    vec2 offset() {
        node *n = parent;
        vec2  o = { 0, 0 };
        while (n) {
            rectd &rect = n->paths.rect;
            o  += rect.xy();
            o  -= n->scroll;
            n   = n->parent;
        }
        return o;
    }
    
    /// override intern (value)
    template <typename T>
    void override(Intern<T> &m, T v) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.d_value         = v;
        internals[m.name] = &m;
    }
    
    /// override extern (default value)
    template <typename T>
    void override(Extern<T> &m, T def) {
        assert(m.name);
        assert(Id<T>() == m.type);
        m.d_value         = def;
        externals[m.name] = &m;
    }
    
    /// todo: Structify these and register them in stack formation, we're looking at an array of these as a model of 'Component'
    struct Members {
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
                            node(cchar_t *cn, str id, Binds b, Elements e);
    virtual                ~node();
            void   standard_bind();
            void  standard_style();
    virtual void            bind();
    virtual void           mount();
    virtual void          umount();
    virtual void         changed(PropList props);
    virtual void           input(str k);
    virtual Element       render();
    virtual void            draw(Canvas &canvas);
    virtual str      format_text();
    bool              processing();
    bool                 process();
    node                   *find(std::string n);
    node           *select_first(std::function<node *(node *)> fn);
    array<node *>         select(std::function<node *(node *)> fn);
    void                    exec(std::function<void(node *)> fn);
    void                   focus();
    void                    blur();
    node                *focused();
    virtual rectd    region_rect(int index, Region &reg, rectd &src, node *n);
    
    /// region to rect calculation done here for nodes
    /// 
    struct RegionOp {
        node::Extern<Region> &reg;
        node           *rel;
        rectd          &rect;
        rectd          &result;
        void operator()(node *n, int index) {
            auto root = n->root ? n->root : n;
            if (reg.style.transitioning()) {
                root->flags      |= node::StyleAnimate;
                rectd      rect0  = rel->region_rect(index, *reg.style.t_start, rect, n);
                rectd      rect1  = rel->region_rect(index, *reg.style.s_value, rect, n);
                real       p      = reg.style.transition_pos();
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
    
    Device &                            device()                            { return Vulkan::device(); }
    ///
    template <typename V> VertexData  vertices(array<V> &v_vbo, int mc)     { return  VertexBuffer<V> (device(), v_vbo, mc); }
    template <typename I> IndexData   polygons(array<I> &v_ibo)             { return   IndexBuffer<I> (device(), v_ibo);     }
    template <typename U> UniformData  uniform(std::function<void(U &)> fn) { return UniformBuffer<U> (device(), fn);        }
    template <typename V> Pipes          model(
                str name, str skin, UniformData ubo, Vertex::Attribs &attr, Asset::Types &atypes, Shaders shaders = null) {
        return Model<V>(name, skin, device().sync(), ubo, attr, atypes, shaders);
    }
    
    Texture texture(Image im, Asset::Type asset) {
        return Texture { &device(), im, asset };
    }
    
    Texture texture(Path path, Asset::Type asset) {
        return texture(Image(path, Image::Rgba), asset);
    }
    
    template <typename V>
    Pipeline<V> texture_view(Texture &tx, UniformData ubo) {
        auto  v_sqr = Vertex::square();
        auto  i_sqr = array<uint32_t> { 0, 1, 2, 2, 3, 0 };
        auto    vbo = vertices(v_sqr, {  });
        auto    ibo = polygons(i_sqr);
        auto assets = Assets {{ Asset::Color, &tx }};
        return Pipeline<V> { device(), ubo, vbo, ibo, assets, "main" };
    }
    
    inline size_t count(str n) {
        for (Element &e: elements)
           for (Bind &b: e.e->binds)
                if (b.id == n)
                    return 1;
        ///
        return 0;
    }
    
    Shared &shared(str name) {
        static Shared s_null;
        node  *n = this;
        while (n)
            if (n->externals.count(name))
                return n->externals[name]->shared_value();
            else if (n->internals.count(name)) /// you can access your own internals from context, and probably only internals flagged as contextual which may just be called, contextual
                return n->internals[name]->shared_value();
            else
                n = n->parent;
        return s_null;
    }
    
    template <typename T>
    T &context(str name) {
        static T t_null;
        Shared sh = shared(name); /// boolean needs to be based on if memory is there, not if its null.
        return sh ? (T &)sh : t_null;
    }
};

#define declare(T)\
    T(str id = null, Binds binds = {}, Elements elements = {}):\
        node(TO_STRING(T), id, binds, elements) { }\
        inline static T *factory() { return new T(); };\
        inline operator  Element() { return { FnFactory(T::factory), id, binds, elements }; }

struct Group:node {
    declare(Group);
};
