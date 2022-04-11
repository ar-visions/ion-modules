import <functional>
import dx:size

export module dx:func;
export {

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

/// courtesy of snawaz@github
template<typename T>
struct function_traits;

template<typename T>
struct count_arg;

template<typename R, typename ...Args>
struct function_traits<std::function<R(Args...)>> {
    static const size_t arg_count = sizeof...(Args);
    typedef R result_type;
    /// get arg type at index, quite fantastic
    template <size_t i>
    struct arg_at {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

///
}