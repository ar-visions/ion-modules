#pragma once
namespace dx {
    template <typename T>
    const T nan() { return std::numeric_limits<T>::quiet_NaN(); };
};
