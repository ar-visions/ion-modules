export module nan;
import std.core;

export {
    template <typename T>
    const T nan() { return std::numeric_limits<T>::quiet_NaN(); };
///
}