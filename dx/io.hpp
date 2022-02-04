#pragma once
#include <string>

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
};

/// you express these with int(var, int, str); U unwraps inside the parenthesis
template <typename T, typename... U>
size_t fn_id(std::function<T(U...)> &fn) {
    typedef T(fnType)(U...);
    fnType ** fp = fn.template target<fnType *>();
    return size_t(*fp);
};

struct io { };

