#pragma once

#include <unordered_map>

/// nullptr verbose, incorrect (nullptr_t is not constrained to pointers at all)
static const nullptr_t null = nullptr;
struct io { };

/// sorry godfather this is the best we can introspect..
///      ...you call this introspection?
template <typename _ZXZtype_name>
const std::string &code_name() {
    static std::string n;
    if (n.empty()) {
        const char str[] = "_ZXZtype_name =";
        const size_t blen = sizeof(str);
        size_t b, l;
        n = __PRETTY_FUNCTION__;
        b = n.find(str) + blen;
        l = n.find("]", b) - b;
        n = n.substr(b, l);
    }
    return n;
}

/// you express these with int(var, int, str); U unwraps inside the parenthesis
template <typename T, typename... U>
size_t fn_id(std::function<T(U...)> &fn) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = fn.template target<fnType *>();
    return (size_t) *fnPointer;
}

struct Type {
    enum Specifier {
        Undefined, /// null state and other uses
        i8,  ui8,
        i16, ui16,
        i32, ui32,
        i64, ui64,
        f32,  f64,
        Bool,
        
        /// high level types.. they got trouble on another level
        Str, Map, Array, Ref, Arb, Node, Lambda, Any,
        
        /// and the mighty general
        Struct
    };
    
    static size_t size(Type::Specifier t) {
        switch (t) {
            case Bool:  return 1;
            case i8:    return 1;
            case ui8:   return 1;
            case i16:   return 2;
            case ui16:  return 2;
            case i32:   return 4;
            case ui32:  return 4;
            case i64:   return 8;
            case ui64:  return 8;
            case f32:   return 4;
            case f64:   return 8;
            default:
                break;
        }
        return 0;
    }
    
    size_t      id        = 0;
    std::string code_name = "";
    
    bool operator==(const Type &b)     const { return id == b.id;      }
    bool operator==(Type &b)           const { return id == b.id;      }
    bool operator==(Type::Specifier b) const { return id == size_t(b); }
    bool operator>=(Type::Specifier b) const { return id >= size_t(b); }
    bool operator<=(Type::Specifier b) const { return id <= size_t(b); }
    bool operator> (Type::Specifier b) const { return id >  size_t(b); }
    bool operator< (Type::Specifier b) const { return id <  size_t(b); }
    bool operator!=(Type::Specifier b) const { return id != size_t(b); }
    inline operator size_t()           const { return id;              }
    inline operator int()              const { return int(id);         }
    ///
    template <typename T>
    T name() {
        return code_name.length() ? code_name.c_str() : specifier_name(id);
    }
    ///
    Type(Specifier id, std::string code_name = "") : id(size_t(id)), code_name(code_name) { }
    Type(size_t    id, std::string code_name = "") : id(id),         code_name(code_name) { }
    
protected:
    const char *specifier_name(size_t id) { /// allow runtime augmentation of this when useful
        static std::unordered_map<size_t, const char *> map = {
            { Undefined, "Undefined"    },
            { i8,        "i8"           }, {   ui8, "ui8"      },
            { i16,       "i16"          }, {  ui16, "ui16"     },
            { i32,       "i32"          }, {  ui32, "ui32"     },
            { i64,       "i64"          }, {  ui64, "ui64"     },
            { f32,       "f32"          }, {   f64, "f64"      },
            { Bool,      "Bool"         }, {   Str, "Str"      },
            { Map,       "Map"          }, { Array, "Array"    },
            { Ref,       "Ref"          }, {  Node, "Node"     },
            { Lambda,    "Lambda"       }, {   Any, "Any"      },
            { Struct,    "Struct"       }
        };
        assert(map.count(id));
        return map[id];
    }
};

template <class T>
struct Id:Type {
    Id() : Type(0) {
        code_name = ::code_name<T>();
        id        = std::hash<std::string>()(code_name);
    }
};

namespace std {
    template<> struct hash<Type> {
        size_t operator()(Type const& id) const { return id; }
    };
}

struct Instances {
    std::unordered_map<Type, void *> allocs; /// definitely want this pattern for var, definitely waiting on that, though.
    inline size_t count(Type &id) { return allocs.count(id); }
    
    template <typename T>
    inline T *get(Type &id) {
        return (T *)allocs[id];
    }
    
    template <typename T>
    inline void set(Type &id, T *ptr) {
        allocs[id] = ptr;
    }
};

/// the smart pointer with baked in arrays and a bit of indirection.
template <typename T>
struct ptr {
    struct Alloc {
        Type              type   = 0;
        std::atomic<long> refs   = 0;
        int               ksize  = 0; // ksize = known size, when we manage it ourselves, otherwise -1 when we are importing a pointer of unknown size
        alignas(16) void *mstart; // imported pointers are not aligned to this spec but our own pointers are.
    } *info;
    ///
    private:
    void alloc(T *&p) {
        size_t   asz = sizeof(Alloc) + sizeof(T);
        info         = (Alloc *)calloc(asz, 1);
        info->type   = Id<T>();
        info->refs   =  2; /// and i thought i saw a two...
        info->ksize  = sizeof(T);
        info->mstart = null;
        p            = &info->mstart;
    }
    public:
    ///
    ptr() : info(null) { }
    ///
    ptr(const ptr<T> &ref) : info(ref.info) {
        if (info)
            info->refs++;
    }
    
    ptr(T *v) {
        size_t   asz = sizeof(Alloc);
        info         = (Alloc *)calloc(asz, 1);
        info->type   = Id<T>();
        info->refs   =  1;
        info->ksize  = -1;
        info->mstart = v; /// imported pointer [mode]
    }
    
    ///
    ptr<T> operator=(T *v) {
        assert(info == null);
        size_t   asz = sizeof(Alloc);
        info         = (Alloc *)calloc(asz, 1);
        info->type   = Id<T>();
        info->refs   =  2; /// and i thought i saw a two...
        info->ksize  = -1;
        info->mstart = v; /// imported pointer [mode]
        return *this;
    }
    ///
    /// todo: should be recycled based on patterns up to 4-8, 64 in each. its just a queue usage
    ~ptr() {
        if (info && --info->refs == 0) {
            if (info->ksize == -1)
                free(info->mstart);
            free(info);
            info = null;
        }
    }
    
    /// effective pointer to the data structure;
    /// for our own they are extensions on the Alloc struct and for imported ones we set the member
    T *effective() const  {
        return (T *)(info ? (info->ksize > 0) ? &info->mstart : info->mstart : null);
        
    }
    ///
    T *operator &()       { return  effective(); }
    T *operator->()       { return  effective(); }
    inline operator T *() { return  effective(); }
    inline operator T &() { return *effective(); }
    
    /// bounds check
    inline T &operator[](size_t index) {
        assert(info->ksize == -1 || index < info->ksize);
        return *ptr()[index];
    }
};

/*

template <typename T>
struct DLNode {
    DLNode  *next = null,
            *prev = null;
    T    *payload = null;
    uint8_t  data[1];
    ///
    static T *alloc() {
        DLNode<T> *b = calloc(1, sizeof(DLNode<T>) + sizeof(T));
         b->payload  = b->data; /// this is a reasonable way to verify some integrity, and we can debug this crazy train
        *b->payload  = T();
        return b;
    }
};

template <typename T>
struct DList {
    DLNode<T> *first = null;
    DLNode<T> *last = null;
};

/// the smart pointer with baked in arrays and a bit of indirection.
template <typename T>
struct list:ptr<DList<T>> {
    DList<T> *m;
    /// -------------------
    list() { alloc(m); }
    T *push() {
        DLNode<T> *n = DLNode<T>::alloc();
        ///
        if (m->last)
            m->last->next = n;
        ///
        n->prev = m->last;
        m->last = n;
        ///
        if (!m->first)
            m->first = n;
        ///
        return T();
    }
    void pop() {
        // release taht memory
        
    }
};

*/
