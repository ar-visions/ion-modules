export module dx:type;

import <cassert>;
import <iostream>;
import <unordered_map>;

import dx:io;
import dx:mappy;

/// this was below, a bit after Type
//import dx:map;
//import dx:var;
//import dx:array;
//import dx:enum;
//import dx:string;
//import dx:rand;

export {

struct string;
struct node;
struct var;
typedef std::function<bool()>                  FnBool;
typedef std::function<void(void *)>            FnArb;
typedef std::function<void(void)>              FnVoid;
typedef std::function<void(var &)>             Fn;
typedef std::function<void(var &, node *)>     FnNode;
typedef std::function<var()>                   FnGet;
typedef std::function<void(var &, string &)>   FnEach;
typedef std::function<void(var &, size_t)>     FnArrayEach;

///
struct Type {
    enum Specifier {
        Undefined, /// null state and arb uses
        i8,  ui8,
        i16, ui16,
        i32, ui32,
        i64, ui64,
        f32,  f64,
        Bool,
        
        /// high level types...
        Str, Map, Array, Ref, Arb, Node, Lambda, Member, Any,
        
        /// and the mighty meta
        Struct
    };
    
    ///
    template <typename T>
    static constexpr Type::Specifier spec(T *v) {
             if constexpr (std::is_same_v<T,       Fn>) return Type::Lambda;
        else if constexpr (std::is_same_v<T,     bool>) return Type::Bool;
        else if constexpr (std::is_same_v<T,   int8_t>) return Type::i8;
        else if constexpr (std::is_same_v<T,  uint8_t>) return Type::ui8;
        else if constexpr (std::is_same_v<T,  int16_t>) return Type::i16;
        else if constexpr (std::is_same_v<T, uint16_t>) return Type::ui16;
        else if constexpr (std::is_same_v<T,  int32_t>) return Type::i32;
        else if constexpr (std::is_same_v<T, uint32_t>) return Type::ui32;
        else if constexpr (std::is_same_v<T,  int64_t>) return Type::i64;
        else if constexpr (std::is_same_v<T, uint64_t>) return Type::ui64;
        else if constexpr (std::is_same_v<T,    float>) return Type::f32;
        else if constexpr (std::is_same_v<T,   double>) return Type::f64;
        else if constexpr (is_map<T>())                 return Type::Map;
        return Type::Undefined;
    }
    
    template <typename T>
    static constexpr Type::Specifier spec() {
             if constexpr (std::is_same_v<T,       Fn>) return Type::Lambda;
        else if constexpr (std::is_same_v<T,     bool>) return Type::Bool;
        else if constexpr (std::is_same_v<T,   int8_t>) return Type::i8;
        else if constexpr (std::is_same_v<T,  uint8_t>) return Type::ui8;
        else if constexpr (std::is_same_v<T,  int16_t>) return Type::i16;
        else if constexpr (std::is_same_v<T, uint16_t>) return Type::ui16;
        else if constexpr (std::is_same_v<T,  int32_t>) return Type::i32;
        else if constexpr (std::is_same_v<T, uint32_t>) return Type::ui32;
        else if constexpr (std::is_same_v<T,  int64_t>) return Type::i64;
        else if constexpr (std::is_same_v<T, uint64_t>) return Type::ui64;
        else if constexpr (std::is_same_v<T,    float>) return Type::f32;
        else if constexpr (std::is_same_v<T,   double>) return Type::f64;
        else if constexpr (is_map<T>())                 return Type::Map;
        return Type::Undefined;
    }
    
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
    
    /// needs a few assertions for runtime safety
    void    lock()                       const {   basics->mx.lock(); }
    void  unlock()                       const { basics->mx.unlock(); }
    
    template <typename T>
    int  compare(T *a, T *b)             const { return basics->fn_compare(a, b); }
    
    template <typename T>
    int  boolean(T *a)                   const { return basics->fn_boolean(a); }
    
    template <typename T>
    void   free(T *ptr, size_t count)    const { return basics->fn_free((void *)ptr, count); }
    
    template <typename T>
    T *alloc(size_t count)               const { return (T *)basics->fn_alloc(count); }
    
        size_t      id    = 0;
    TypeBasics *basics    = null;

    size_t   sz()                        const {
        assert(!basics); /// this should be the null one for cases designed
        return size(Specifier(id));
    }
    
    bool operator==(const     Type &b) const { return id == b.id;      }
    bool operator==(Type           &b) const { return id == b.id;      }
    bool operator==(Type::Specifier b) const { return id == size_t(b); }
    ///
    bool operator!=(const Type     &b) const { return id != b.id;      }
    bool operator!=(Type           &b) const { return id != b.id;      }
    bool operator!=(Type::Specifier b) const { return id != size_t(b); }
    ///
    bool operator>=(Type::Specifier b) const { return id >= size_t(b); }
    bool operator<=(Type::Specifier b) const { return id <= size_t(b); }
    bool operator> (Type::Specifier b) const { return id >  size_t(b); }
    bool operator< (Type::Specifier b) const { return id <  size_t(b); }
    ///
    inline operator size_t()           const { return id;              }
    inline operator int()              const { return int(id);         }
    inline operator bool()             const { return id > 0;          }
    
    ///
    template <typename T>
    T name() { return basics ? basics->code_name.c_str() : specifier_name(id); }
    
    /// could potentially use an extra flag for knowing origin
    Type(Specifier id, TypeBasics *basics = null) : id(size_t(id)), basics(basics) { }
    Type(size_t    id, TypeBasics *basics = null) : id(id),         basics(basics) { }
    Type() : basics(null) { }
    
protected:
    const char *specifier_name(size_t id) { /// allow runtime augmentation of this when useful
        static std::unordered_map<size_t, const char *> map2 = {
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
        assert(map2.count(id));











        return map2[id];
    }
};

template <class T>
struct Id:Type {
    Id() : Type(0) {
        basics    = &(TypeBasics &)type_basics<T>();
        id        = std::hash<std::string>()(basics->code_name);
        Type::Specifier ts = Type::spec<T>();
        if (ts >= i8 && ts <= Array) { id = ts; }
    }
};

namespace std {
    template<> struct hash<Type>      { size_t operator()(Type const& id) const { return id; } };
}

/// definitely want this pattern for var
struct Instances {
    std::unordered_map<Type, void *> allocs;
    inline size_t count(Type &id)     { return allocs.count(id); }
    
    template <typename T>
    inline T *get(Type &id)           { return (T *)allocs[id]; }
    
    template <typename T>
    inline void set(Type &id, T *ptr) { allocs[id] = ptr; }
};

/// deprecate usage, convert style.
template <typename T>
struct ptr {
    struct Alloc {
        Type                     type   = 0;
        std::atomic<long>        refs   = 0;
        int                      ksize  = 0; // ksize = known size, when we manage it ourselves, otherwise -1 when we are importing a pointer of unknown size
        alignas(16) void        *mstart;     // imported pointers are not aligned to this spec but our own pointers are.
    } *info;
    ///
    private:
    void alloc(T *&ptr) {
        size_t   asz       = sizeof(Alloc) + sizeof(T);
        info               = (Alloc *)calloc(asz, 1);
        info->type         = Id<T>();
        info->refs         =  2; /// and i thought i saw a two...
        info->ksize        = sizeof(T);
        info->mstart       = null;
        ptr                = &info->mstart;
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

//void _print(str t, array<var> &a, bool error);

///
struct is_apple :
#if defined(__APPLE__)
    std::true_type
#else
    std::false_type
#endif
{};
    
///
struct is_android :
#if defined(__ANDROID_API__)
    std::true_type
#else
    std::false_type
#endif
{};
    
///
struct is_win :
#if defined(_WIN32) || defined(_WIN64)
    std::true_type
#else
    std::false_type
#endif
{};
    
///
struct is_gpu : std::true_type {};

///
struct is_debug :
#if !defined(NDEBUG)
    std::true_type
#else
    std::false_type
#endif
{};

#if !defined(WIN32) && !defined(WIN64)
#define UNIX /// its a unix system.  i know this.
#endif

/// adding these declarations here.  dont mind me.
enum KeyState {
    KeyUp,
    KeyDown
};

struct KeyStates {
    bool shift;
    bool ctrl;
    bool meta;
};

struct KeyInput {
    int key;
    int scancode;
    int action;
    int mods;
};

enum MouseButton {
    LeftButton,
    RightButton,
    MiddleButton
};

/// needs to be ex, but no handlers written yet
/*
enum KeyCode {
    Key_D       = 68,
    Key_N       = 78,
    Backspace   = 8,
    Tab         = 9,
    LineFeed    = 10,
    Return      = 13,
    Shift       = 16,
    Ctrl        = 17,
    Alt         = 18,
    Pause       = 19,
    CapsLock    = 20,
    Esc         = 27,
    Space       = 32,
    PageUp      = 33,
    PageDown    = 34,
    End         = 35,
    Home        = 36,
    Left        = 37,
    Up          = 38,
    Right       = 39,
    Down        = 40,
    Insert      = 45,
    Delete      = 46, // or 127 ?
    Meta        = 91
};
*/
///
}


