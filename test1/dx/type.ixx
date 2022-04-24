export module type;

import io;
import map;

export {

struct string;
struct node;
struct var;

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


