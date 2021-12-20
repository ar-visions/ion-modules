#pragma once
#include <dx/dx.hpp>
#include <ux/ux.hpp>
#include <ux/region.hpp>
#include <vk/vk.hpp>

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

struct node;

struct IProps {
    //void enumerate(std::function<void(String &, var::Type)> fn) {
    //}
};

struct Composer;
typedef std::function<void *(Composer *, var &)> FnComposerData;
typedef std::function<void *(Composer *, Args &)> FnComposerArgs;
typedef map<std::string, node *>     NodeMap;
struct Construct;

struct Definition {
    str id;
    void *ptr;
    var data; //default value
    Fn fn;
    FnComposerData setter;
    Definition() { }
    Definition(str id, void *ptr, Fn fn) : id(id), ptr(ptr), fn(fn) { }
    virtual ~Definition() { }
};

typedef map<std::string, Definition> DefMap;
typedef map<std::string, Args>       Nodes;
typedef map<std::string, node *>     State;

void delete_node(node *n);

struct Border {
    double          size  = 0;
    rgba            color = "#000";
    bool            dash  = false;
    Border()        { }
    void copy(const Border &b) {
        size  = b.size;
        color = b.color;
        dash  = b.dash;
    }
    void import_data(var &d) {
        size  = double(d["size"]);
        color =   rgba(d["color"]);
        dash  =   bool(d["dash"]);
    }
    var export_data() {
        return Args {
            {"size",  size},
            {"color", color},
            {"dash",  dash}
        };
    }
    serializer(Border, size > 0 && color.a > 0);
};

struct Text {
    str             label = "";
    rgba            color = "#000";
    Vec2<Align>     align = {Align::Middle, Align::Middle};
    
    Text() { }
    
    void copy(const Text &b) {
        label = b.label;
        color = b.color;
        align = b.align;
    }
    
    void import_data(var &d) {
        label  =  str(d["label"]);
        color  = rgba(d["color"]);
        align  = Vec2<Align>(d["align"]);
    }
    
    var export_data() {
        return Args {
            {"label", var(label)},
            {"color", color},
            {"align", align}
        };
    }
    
    serializer(Text, label != "" && color.a > 0);
};

struct Fill {
    rgba            color = null;
    Fill()          { }
    operator bool()  { return color.a > 0; }
    bool operator!() { return !(bool()); }
    
    void copy(const Fill &b) {
        color = b.color;
    }
    void import_data(var &d) {
        color = rgba(d["color"]);
    }
    var export_data() {
        return Args {{"color", color}};
    }
    serializer(Fill, color.a > 0);
};

typedef vec<str> &PropList;

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

typedef std::function<vec<Attrib>(str)> AttribFn;

struct node {
    const char    *selector = "";
    const char    *class_name = "";
    node          *parent = null;
    map<std::string, node *> mounts;
    Args           args;
    var            intern;
    var            expose;
    vec<Element>   elements;
    DefMap         defs;
    State          smap;
    std::queue<Fn> queue;
    Path           path; // i see no reason to have 'path' associated to a clip area, thats just too complicated to implement fully
    bool           mounted = false;
    node          *root = nullptr;
    double         y_scroll = 0;
    map<std::string, PipelineData> objects;
    
    template <typename T>
    void exposed(str name, T &loc, T value) {
        expose[name]  = var(value);
    }
    
    template <typename T>
    void internal(str name, T &loc, T value) {
        intern[name]  = var(value);
    }

    /// node needs a way to update uniforms per frame
    struct Props:IProps {
        str         id;
        str         bind;
        int         tab_index;
        Text        text;
        Fill        fill;
        Border      border;
        Region      region;
    } props;
                    node(nullptr_t n = null);
                    node(const char *selector, const char *cn, Args pdata, vec<Element> elements);
    virtual        ~node();
            void    define_standard();
    virtual void    define();
    virtual void    mount();
    virtual void    umount();
    virtual void    changed(PropList props);
    virtual void    input(str k);
    virtual Element render();
    var             get_state(std::string n);
    var             set_state(std::string n, var v);
    virtual void    draw(Canvas &canvas);
    virtual str     format_text();
    bool            processing();
    bool            process();
    node           *find(std::string n);
    node           *select_first(std::function<node *(node *)> fn);
    vec<node *>     select(std::function<node *(node *)> fn);
    virtual rectd   calculate_rect(node *child);
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
        /*
        auto obj = Obj<V>(path, [&](auto &g, vec3 &p, vec2 &u, vec3 &n) -> V {
            return V(p, n, u);
        });
        auto vbo = vertices<V>(obj.vbo);
        auto res = PipelineMap(obj.groups.size());
        for (auto &[n, group]: obj.groups) {
            size_t cn = shaders.count(n);
            size_t ca = shaders.count("*");
            auto   sh = cn ? shaders[n]   :
                        ca ? shaders["*"] : str(n);
            auto  ibo = polygons<uint32_t>(group.ibo);
            res[n]    = Pipeline<V> { *vbo.device, ubo, vbo, ibo, attr, sh };
            console.log("group: {0}, shader: {1}", {n, sh});
        }
        return res;*/
        return {};
    }
    
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
        for (auto &e: elements) {
            if (e.args.count("id") && e.args["id"] == n)
                return 1;
        }
        return 0;
    }
    
    var &context(str field) {
        static var d_null;
        node *n = this;
        while (n)
            if (n->expose.count(field))
                return n->expose[field];
            else
                n = n->parent;
        return intern.count(field) ? intern[field] : d_null;
    }
};

#define declare(T)\
    T(Args args = {}, vec<Element> elements = {}):\
        node((const char *)"", (const char *)TO_STRING(T), args, elements) { }\
        inline static T *factory() { return new T(); };\
        inline operator Element() { return { FnFactory(T::factory), args, elements }; }


struct Group:node {
    declare(Group);
};

