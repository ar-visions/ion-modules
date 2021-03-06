#pragma once
#include <string>
#include <thread>
#include <vector>

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
    std::function<void *(size_t, void *)> fn_alloc_array;
    std::function<int   (void *, void *)> fn_compare;
};

template<typename>
struct is_func                   : std::false_type {};

template<typename T>
struct is_func<std::function<T>> : std::true_type  {};

template <class U, class T>
struct can_convert {
  enum { value = std::is_constructible<T, U>::value && !std::is_convertible<U, T>::value };
};


#include <type_traits>
#include <utility>

/// has_operator_equal<T>::value == bool
template<class T, class EqualTo>
struct has_operator_equal_impl
{
    template<class U, class V>
    static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
    template<typename, typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template<class T, class EqualTo = T>
struct has_operator_equal : has_operator_equal_impl<T, EqualTo>::type {};


template <typename _ZZion>
const TypeBasics &type_basics() {
    typedef _ZZion T;
    static TypeBasics tb;
    if (tb.code_name.empty()) {
        const char    s[] = "_ZZion =";
        const size_t blen = sizeof(s);
              size_t b;
              size_t l;
        tb.code_name  = __PRETTY_FUNCTION__;
        b             = tb.code_name.find(s) + blen;
        l             = tb.code_name.find("]", b) - b;
        tb.code_name  = tb.code_name.substr(b, l);
        tb.fn_boolean = [](void *va) -> bool {
            if constexpr (can_convert<T, bool>::value) {
                static T s_null;
                T &a = *(va ? (T *)va : (T *)&s_null);
                return bool(a);
            }
            return true;
        };
        ///
        tb.fn_compare = [](void *va, void *vb) -> int {
            if constexpr (is_func<T>() || has_operator_equal<T>::value) {
                static T s_null;
                T &a = *(va ? (T *)va : (T *)&s_null);
                T &b = *(vb ? (T *)vb : (T *)&s_null);
                if constexpr (is_func<T>())
                    return int(fn_id((T &)a) == fn_id((T &)b));
                else
                    return int(a == b);
            }
            return va == vb;
        };
        ///
        /// express Orbiter..
        ///
        tb.fn_free = [](void *ptr, size_t count) -> void {
                 if (count > 1) delete[] (T *)(ptr);
            else if (count    ) delete   (T *)(ptr);
        };
        //typedef std::vector<T> vt;
        ///
        tb.fn_alloc       = [](size_t c)          -> void * { return (void *)new T(); };
        tb.fn_alloc_array = [](size_t c, void *d) -> void * {
            //std::vector<bool> hi = { true };
            //bool *b = hi.data();
            //vt *v = new vt();
            //v->reserve(c);
            //if (d)
            //    memcpy((void *)v->data(), d, c * sizeof(T));
            //return v;
            return null;
        };
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

/// sine function
template <typename T>
T sin(T x) {
    const int max_iter = 70;
    T c = x, a = 1, f = 1, p = x;
    const T eps = 1e-8;
    ///
    for (int i = 1; (a < 0 ? -a : a) > eps and i <= max_iter; i++) {
        f *= (2 * i) * (2 * i + 1);
        p *= -1 * x  * x;
        a  =  p / f;
        c +=  a;
    }
    return c;
}

/// cosine function (sin - pi / 2)
template <typename T>
T cos(T x) { return sin(x - M_PI / 2.0); }

/// tan, otherwise known as sin(x) / cos(y), the source
template <typename T>
T tan(T x) { return sin(x) / cos(x); }

struct io { };

