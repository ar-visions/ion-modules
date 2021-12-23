#pragma once
#include <dx/dx.hpp>
#include <ux/ux.hpp>
#include <ux/region.hpp>
#include <vk/vk.hpp>

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

struct node;

struct Composer;
typedef std::function<void *(Composer *, var &)> FnComposerData;
typedef std::function<void *(Composer *, Args &)> FnComposerArgs;
typedef map<std::string, node *>     NodeMap;
struct Construct;

typedef map<std::string, node *>     State;
typedef vec<str> &PropList;
typedef std::function<vec<Attrib>(str)> AttribFn;

void delete_node(node *n);

template <typename _ZXZtype_name>
const std::string &type_name() {
    static std::string n;
    if (n.empty()) {
        const char str[] = "_ZXZtype_name =";
        const size_t blen = sizeof(str);
        size_t b, l;
        n = __PRETTY_FUNCTION__;
        b = n.find(str) + blen + 1;
        l = n.find("]", b) - b;
        n = n.substr(b, l);
    }
    return n;
}

template <typename T, typename... U>
size_t fn_id(std::function<T(U...)> fn) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = fn.template target<fnType *>();
    return (size_t) *fnPointer;
}

struct node {
    enum Flags {
        Focused  = 1,
        Captured = 2
    };
    const char    *selector   = "";
    const char    *class_name = "";
    node          *parent     = null;
    map<std::string, node *> mounts;
    vec<Element>   elements; /// used to check differences
    std::queue<Fn> queue;
    Path           path;
    bool           mounted    = false;
    node          *root       = nullptr;
    double         y_scroll   = 0; // store indirect
    Binds          binds;
    int32_t        flags;
    map<std::string, PipelineData> objects;
    
    /// member data structure
    struct Member {
        enum Type {
            Undefined,
            Context, // context is a direct value at member, such as id
            Intern,
            Extern
        };
        size_t   type_hash = 0;
        Type     type      = Undefined;
        str      name      = null;
        size_t   cache     = 0;
     ///var      data      = null; /// when we lose type context we resort to serialization
        Fn       fn_var_set;
        FnFilter fn_var_get;
        FnArb    fn_type_set;
        FnVoid   fn_unset;
        
        ///
        size_t peer_cache = 0xffffffff;
        Member *v_member  = null;
        
        bool operator != (Member &v) {
            return !(operator==(v));
        }
        bool operator == (Member &v) {
            return v_member == &v && peer_cache == v.cache;
        }
        void operator=(Member &v) {
            if (type_hash == v.type_hash)
                fn_type_set((void *)&v);
            else {
                var p = null;
                var conv0 = v.fn_var_get(p);
                fn_var_set(conv0);
            }
            v_member   = &v;
            peer_cache = v.cache;
        }
        void unset() {
            fn_unset();
        }
    };
    
    ///
    map<str, Member *> contextuals;
    map<str, Member *> internals;
    map<str, Member *> externals;
    
    /// member type-specific struct
    template <typename T, const Member::Type MT>
    struct MType:Member {
        size_t cache = 0;
        T        def;
        T      value;
        ///
        void init() {
            type_hash  = std::hash<std::string>()(::type_name<T>());
            /// function sets, unset
            fn_unset   = [&]() { cache = 0; };
            /// line in the sand here. the need to know if a class is var'able is, quite needed here.
            /// that means we need an io base class just as a pure indication that this happens
            if constexpr (std::is_base_of<T, io>()) {
                fn_var_get = [&](var &) {
                    return var(value);
                };
                fn_var_set = [&](var &v) {
                    if constexpr (std::is_same_v<T, Fn>()) {
                        T conv = T(v);
                        if (fn_id(value) != fn_id(conv)) {
                            value  = v;
                            cache++;
                        }
                    }/* else if (!(value == conv)) {
                        value  = conv;
                        cache++;
                    }*/
                };
            }
            fn_type_set = [&](void *pv) {
                T &v = *(T *)pv;
                if constexpr (std::is_same_v<T, Fn>) {
                    if (fn_id(value) != fn_id(v)) {
                        value  = v;
                        cache++;
                    }
                }
            };
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
        T &operator=(T  &v) {
            fn_type_set((void *)&v);
            return value;
        }
        ///
        T &ref()         { return cache ? value : def; }
           operator T&() { return ref(); }
        T &operator() () { return ref(); }
        operator bool () { return ref(); }
    };
    
    template <typename T>
    struct Context:MType<T, Member::Context> {
        inline void operator=(T v) {
            Member::fn_type_set((void *)&v);
        }
    };
    
    template <typename T>
    struct Intern:MType<T, Member::Intern> {
        inline void operator=(T v) {
            Member::fn_type_set((void *)&v);
        }
    };
    
    template <typename T>
    struct Extern:MType<T, Member::Extern> {
        inline void operator=(T v) {
            Member::fn_type_set((void *)&v);
        }
    };
    
    struct Border {
        Extern<double>  size;
        Extern<rgba>    color;
        Extern<bool>    dash;
        operator bool() { return size() > 0 && color().a > 0; }
    };
    
    struct Text {
        Extern<str>     label;
        Extern<rgba>    color;
        Extern<AlignV2> align;
        operator bool() { return label() && color().a > 0; }
    };
    
    struct Fill {
        Extern<rgba>    color;
        operator bool()  {
            return color().a > 0;
        }
    };
    
    template <typename T>
    void contextual(str name, Context<T> &m, T def) {
        m.name          = name;
        m.def           = def;
        contextuals[name] = &m;
    }
    
    ///
    template <typename T>
    void internal(str name, Intern<T> &m, T def) {
        m.name          = name;
        m.def           = def;
        internals[name] = &m;
    }
    
    ///
    template <typename T>
    void external(str name, Extern<T> &m, T def) {
        m.name          = name;
        m.def           = def;
        externals[name] = &m;
    }
    
    /// node needs a way to update uniforms per frame
    struct Members {
        Context<str>   id;
        Extern<str>    bind;
        Extern<int>    tab_index;
        Text           text;
        Fill           fill;
        Border         border;
        Extern<Region> region;
    } m;
                    node(nullptr_t n = null);
                    node(const char *selector, const char *cn, Binds binds, vec<Element> elements);
    virtual        ~node();
            void    standard();
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
    virtual rectd   calculate_rect(node *child);
    void            focus();
    void            blur();
    node           *focused();
    
    int             u_next_id = 0;
    
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
                      vec<Attrib>  attr, ShaderMap    shaders = { }) {
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

