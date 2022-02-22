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
struct Shared2 {
    int x;
    operator bool() { return true; }
};

/// reduce Bind, translate the member to member::shared value
///
struct Bind {
    str       id;
    Shared    shared;
    
    Bind(str id, Member &m) : id(id), shared(m.shared) { }
    
    template <typename T>
    Bind(str id, T v)       : id(id), shared(1, new T(v)) { }
    
    bool operator==(Bind &b) { return id == b.id; }
    bool operator!=(Bind &b) { return !operator==(b); }
};

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

struct node {
    enum Flags {
        Focused      = 1,
        Captured     = 2,
        StateUpdate  = 4,
        StyleAnimate = 8
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
    //map<str, Member *> stationaries; /// it works but it needs a bit more method around it in usefulness
    map<str, Member *> internals;
    map<str, Member *> externals;
    
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
            T             t_start      = T();
            T             t_value      = T();
            T            *s_value      = null;
            Alloc<T>      d_value;
            
            ///
            T &current() {
                assert(host);
                return host->shared    ? *(T *)host->shared :
                      (transitioning() ?  t_value :
                       s_value         ? *s_value :
                                         *(T *)d_value);
            }
            
            ///
            T &auto_update() {
                assert(host);
                if (!host->shared && transitioning() && !manual_trans)
                    t_value = (*trans)(t_start, *s_value, transition_pos());
                return current();
            }
            
            void manual_transition(bool manual) { manual_trans = manual; }
            bool transitioning()  const { return (!trans || trans->type == StTrans::None) ? false : (timer_start + trans->duration.millis) > ticks(); }
            real transition_pos() const {
                return (!trans || trans->type == StTrans::None) ? 1.0 :
                         std::clamp((ticks() - timer_start) / real(trans->duration.millis), real(0.0), real(1.0));
            }
            
            inline void changed(T &snapshot, T *s_value, StTrans *t) {
                if (t != trans) {
                    t_start     = snapshot;
                    timer_start = ticks();
                    trans       = t;
                }
                this->s_value   = s_value;
            }
        };
        size_t        cache = 0;
        Alloc<T>      d_value;
        StyleValue    style;
        
        /// this cannot use style, because its used by style
        bool state_bool() {
            if (!shared)
                return false;
            T v = *(T *)shared;
            if constexpr (std::is_same_v<T, std::filesystem::path>)
                return std::filesystem::exists(v);
            else
                return bool(v);
        }
        virtual T &current() { return  style.auto_update(); }
        void      *ptr()     { return &current(); }
    
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
                                /// it could be configured to fault here
                                var conv = p->value;
                                p->instances.set(m.type, new T(conv));
                            }
                        }
                        mm.style_value_set(p ? p->instances.get<T>(m.type) : null, p ? &p->trans : null);
                    };
                else
                    lambdas->compute_style = null;
                
                lambdas->type_set = [](Member &m, Shared &shared) -> void {
                    NMember<T,MT> &mm  =  (NMember<T,MT> &)m;
                    if constexpr (is_func<T>()) {
                        T *fv = shared.memory<T>();
                        if (fn_id(*(T *)mm.shared) != fn_id(*fv)) {
                            mm.shared = *fv;
                            mm.cache++;
                        }
                    } else if (mm.shared != shared) { /// the check is needed for cache (peer-cache used to determine when to update)
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
                            T    conv = T(sz);
                            for (auto &i: v.a)
                                conv += static_cast<typename T::value_type>(i); /// static_cast not required, i think.  thats why its here.
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
    
    /* was used for id and nothing else.. sort of didnt even want the idea. stationaries always end up in a drawer for a decade
    template <typename T>
    struct Stationary:NMember<T, Member::Stationary> {
        inline void operator=(T v) {
            assert(Member::lambdas_map[type].type_set != null);
            Member::lambdas_map[type].type_set(*this, (void *)&v);
        }
    };*/
    
    ///
    template <typename T>
    struct Intern:NMember<T, Member::Intern> {
        typedef T value_type;
        inline void operator=(T v) {
            Shared sh = Alloc<T>(v);
            Member::lambdas->type_set(*this, sh);
        }
    };
    
    ///
    template <typename T, typename C>
    struct Lambda:Intern<T> {
        typedef T value_type;
        std::function<T(C &)> lambda;
        ///
        T &current() {
            T *v = new T(lambda(*(C *)Intern<T>::arg));
            Member::shared = v;
            return (T &)Member::shared;
        }
    };
    
    ///
    template <typename T>
    struct Extern:NMember<T, Member::Extern> {
        typedef T value_type;
        inline void operator=(T v) {
            Shared sh = Alloc<T>(v);
            Member::lambdas->type_set(*this, sh);
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
    /*
    template <typename T>
    void stationary(str name, Stationary<T> &nmem, T def) {
        nmem.type          =  Id<T>();
        nmem.member        =  Member::Stationary;
        nmem.name          =  name;
        nmem.def           =  def;
        nmem.arg           = this; // should only be set here friends
        stationaries[name] = &nmem;
    }*/
    
    template <typename T>
    void internal(str name, Intern<T> &nmem, T def) {
        nmem.type          = Id<T>();
        nmem.member        = Member::Intern;
        nmem.name          = name;
        nmem.d_value       = new T(def);
        nmem.arg           = this;
        internals[name]    = &nmem;
        nmem.integrate();
    }
    
    template <typename R, typename T>
    void lambda(str name, Lambda<R,T> &nmem, std::function<R(T &)> fn) {
        nmem.type          = Id<R>();
        nmem.member        = Member::Intern;
        nmem.name          = name;
        nmem.lambda        = std::function<R(T &)>(fn);
        nmem.arg           = this;
        internals[name]    = &nmem;
        nmem.integrate();
    }
    
    template <typename T>
    void external(str name, Extern<T> &nmem, T def) {
        nmem.type          = Id<T>();
        nmem.member        = Member::Extern;
        nmem.name          = name;
        nmem.d_value       = new T(def);
        nmem.arg           = this;
        externals[name]    = &nmem;
        nmem.integrate();
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
    
    struct Members {
        //Stationary<str> id;
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
                            node(std::nullptr_t n);
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
    struct RegionOp {
        node::Extern<Region> &reg;
        node           *rel;
        rectd          &rect;
        rectd          &result;
        void operator()(node *n, int index) {
            auto root = n->root ? n->root : n;
            if (reg.style.transitioning()) {
                root->flags      |= node::StyleAnimate;
                rectd      rect0  =  rel->region_rect(index,  reg.style.t_start, rect, n);
                rectd      rect1  =  rel->region_rect(index, *reg.style.s_value, rect, n);
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
    
    inline size_t count(str n) {
        for (Element &e: elements)
           for (Bind &b: e.e->binds)
                if (b.id == n)
                    return 1;
        ///
        return 0;
    }
    
    /// context was using a 'stationary' concept, a third external effectively which "id" was part of
    /// i dont believe its as useful as a general external link, same properties as exposed through member via element map
    template <typename T>
    T &context(str name) {
        static T t_null = null;
        node  *n = this;
        while (n)
            if (n->externals.count(name))
                return ((Extern<T> *)n->externals[name])->value;
            else
                n = n->parent;
        return t_null;
    }
};

// trying to not use the binds on node* memory, but rather the element.  i dont think its anything other than a copy (?)

#define declare(T)\
    T(str id = null, Binds binds = {}, Elements elements = {}):\
        node(TO_STRING(T), id, binds, elements) { }\
        inline static T *factory() { return new T(); };\
        inline operator  Element() { return { FnFactory(T::factory), id, binds, elements }; }

struct Group:node {
    declare(Group);
};
