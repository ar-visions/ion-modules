#pragma once
#include <string>
#include <thread>

typedef const char cchar_t;

/// nullptr verbose, incorrect (std::nullptr_t is not constrained to pointers at all)
static const std::nullptr_t null = nullptr;

#if INTPTR_MAX == INT64_MAX
typedef double       real;
#elif INTPTR_MAX == INT32_MAX
typedef float        real;
#else
#error Unknown pointer size or missing size macros!
#endif

typedef float        r32;
typedef double       r64;

/// all types have a mutex, and a code name... [oh grow up double-oh-seven]
struct TypeBasics {
    std::string code_name;
    std::mutex  mx;
    ///
    /// constructor and destructor for type!
    /// the dissenting voice with std::shared_ptr makes sense.
    /// shared != shared_two should perform a value compare. shared.ptr() != shared_two.ptr() would be the 'std::shared_ptr' method, sort of.
    /// ---------------------------------------------------
    /// prefering to use this in array and str in the most integrated fashion
    /// this is more language agnostic than other approaches and is central to either vector or scalar allocation
    std::function<void  (void *, size_t)> fn_free;
    std::function<bool  (void *        )> fn_boolean;
    std::function<void *(        size_t)> fn_alloc;
    std::function<int   (void *, void *)> fn_compare;
};

template<typename>
struct is_func                   : std::false_type {};

template<typename T>
struct is_func<std::function<T>> : std::true_type  {};


template <typename _ZZionm>
const TypeBasics &type_basics() {
    typedef _ZZionm T;
    static TypeBasics tb;
    if (tb.code_name.empty()) {
        const char    s[] = "_ZZionm =";
        const size_t blen = sizeof(s);
              size_t b;
              size_t l;
        tb.code_name = __PRETTY_FUNCTION__;
        b = tb.code_name.find(s) + blen;
        l = tb.code_name.find("]", b) - b;
        tb.code_name = tb.code_name.substr(b, l);
 
        //
        // silver: able to cast into lambdas, especially when its just a pointer
        // otherwise its lexical hijack of the token vars into converted ones
        //
        tb.fn_boolean = [](void *va) -> bool {
            static T s_null;
            T &a = *(va ? (T *)va : (T *)&s_null);
            return bool(a);
        };
        
        tb.fn_compare = [](void *va, void *vb) -> int {
            static T s_null;
            T &a = *(va ? (T *)va : (T *)&s_null);
            T &b = *(vb ? (T *)vb : (T *)&s_null);
            if constexpr (is_func<T>())
                return int(fn_id((T &)a) == fn_id((T &)b));
            else
                return int(a == b);
        };
        ///
        tb.fn_free = [](void *ptr, size_t count) -> void {
                 if (count > 1) delete[] (T *)(ptr);
            else if (count    ) delete   (T *)(ptr);
        };
        ///
        tb.fn_alloc = [](size_t count) -> void * { return (void *)new T(); };
    }
    return tb;
};

/// you express these with int(var, int, str); U unwraps inside the parenthesis
template <typename T, typename... U>
size_t fn_id(std::function<T(U...)> &fn) {
    typedef T(fnType)(U...);
    fnType ** fp = fn.template target<fnType *>();
    return size_t(*fp);
};

struct io { };

