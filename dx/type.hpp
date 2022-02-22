#pragma once

#include <unordered_map>
#include <cstddef>
#include <atomic>
#include <string>
#include <dx/io.hpp>
#include <dx/array.hpp>
#include <dx/map.hpp>

/// bringing these along
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

/// rename all others to something in their domain.
struct Type {
    enum Specifier {
        Undefined, /// null state and other uses
        i8,  ui8,
        i16, ui16,
        i32, ui32,
        i64, ui64,
        f32,  f64,
        Bool,
        
        /// high level types...
        Str, Map, Array, Ref, Arb, Node, Lambda, Any,
        
        /// and the mighty general
        Struct
    };
    
    /// thin-function not generally suited
    template <typename T>
    static Type::Specifier specifier(T *v) {
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
    
    static size_t size(Type::Specifier t) { // needs to be on the instance
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
    
    size_t sz() {
        assert(code_name == "");
        return size(Specifier(id));
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
    inline operator bool()             const { return id > 0;          }
    
    ///
    template <typename T>
    T name() { return code_name.length() ? code_name.c_str() : specifier_name(id); }
    
    /// could potentially use an extra flag for knowing origin
    Type(Specifier id, std::string code_name = "") : id(size_t(id)), code_name(code_name) { }
    Type(size_t    id, std::string code_name = "") : id(id),         code_name(code_name) { }
    Type() { }
    
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
        T    *ptr = null;
        Type::Specifier ts = Type::specifier(ptr);
        if (ts >= i8 && ts <= Array) {
            id = ts;
            int test = 0;
            test++;
        }
    }
};

namespace std {
    template<> struct hash<Type> {
        size_t operator()(Type const& id) const { return id; }
    };
}

/// definitely want this pattern for var
struct Instances {
    std::unordered_map<Type, void *> allocs;
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
/// think about its integration with var.. i somehow missed the possibility yesterday.
/// its ridiculous not to support vector here too.
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

#include <dx/map.hpp>
#include <dx/var.hpp>
#include <dx/array.hpp>
#include <dx/enum.hpp>
#include <dx/string.hpp>
#include <dx/rand.hpp>

#define ex_shim(C,E,D) \
    static Symbols symbols;\
    C(E t = D):ex<C>(t) { }\
    C(string s):ex<C>(D) { kind = resolve(s); }

#define enums(C,E,D,ARGS...) \
    struct C:ex<C> {\
        static Symbols symbols;\
        enum E { ARGS };\
        C(E t = D):ex<C>(t) { }\
        C(string s):ex<C>(D) { kind = resolve(s); }\
    };\

#define io_shim(C,E) \
    C(std::nullptr_t n) : C()  { }                      \
    C(const C &ref)            { copy(ref);            }\
    C(var &d)                  { importer(d);          }\
    operator var()             { return exporter();    }\
    operator bool()  const     { return   E;           }\
    bool operator!() const     { return !(E);          }\
    C &operator=(const C &ref) {\
        if (this != &ref)\
            copy(ref);\
        return *this;\
    }

void _print(str t, array<var> &a, bool error);

///
/// best to do this kind of thing in logic that exists with the 'others'
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


template <typename T>
static T input() {
    T v;
    std::cin >> v;
    return v;
}

enum LogType {
    Dissolvable,
    Always
};

/// could have a kind of Enum for LogType and LogDissolve, if fancied.
typedef std::function<void(str &)> FnPrint;

template <const LogType L>
struct Logger {
    FnPrint printer;
    Logger(FnPrint printer = null) : printer(printer) { }
    ///
protected:
    void intern(str &t, array<var> &a, bool err) {
        t = var::format(t, a);
        if (printer != null)
            printer(t);
        else {
            auto &out = err ? std::cout : std::cerr;
            out << std::string(t) << std::endl << std::flush;
        }
    }

public:
    
    /// print always prints
    inline void print(str t, array<var> a = {}) { intern(t, a, false); }

    /// log categorically as error; do not quit
    inline void error(str t, array<var> a = {}) { intern(t, a, true); }

    /// log when debugging or LogType:Always, or we are adapting our own logger
    inline void log(str t, array<var> a = {}) {
        if (L == Always || printer || is_debug())
            intern(t, a, false);
    }

    /// assertion test with log out upon error
    inline void assertion(bool a0, str t, array<var> a = {}) {
        if (!a0) {
            intern(t, a, true);
            assert(a0);
        }
    }

    /// log error, and quit
    inline void fault(str t, array<var> a = {}) {
        _print(t, a, true);
        assert(false);
        exit(1);
    }

    /// prompt for any type of data
    template <typename T>
    inline T prompt(str t, array<var> a = {}) {
        _print(t, a, false);
        return input<T>();
    }
};

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
extern Logger<LogType::Dissolvable> console;



