#pragma once

template <typename T>
struct FlagsOf:io {
    uint32_t flags = 0;
    FlagsOf(std::nullptr_t = null) { }
    FlagsOf(std::initializer_list<T> l) {
        for (auto i:l)
            flags |= i;
    }
    FlagsOf(T f) : flags(uint32_t(f))   { }
    bool operator()  (T c) { return (flags & c) == c; }
    void operator += (T f) { flags |=  f; }
    void operator -= (T f) { flags &= ~f; }
    operator var() { return flags; }
    FlagsOf(var &v) {
    }
};
