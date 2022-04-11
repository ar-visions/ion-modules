import dx:io;
export module dx:flags;
export {

template <typename T>
struct FlagsOf:io {
    uint32_t flags = 0;
    ///
    FlagsOf(std::nullptr_t = null) { }
    FlagsOf(std::initializer_list<T> l) {
        for (auto v:l)
            flags |= flag_for(v);
    }
    FlagsOf (T f) : flags(uint32_t(f))  { }
    FlagsOf (var &v)                    { flags = uint32_t(v);   }
    operator   var()                    { return flags;          }
    operator  bool()                    { return flags != 0;     }
    uint32_t flag_for(T v)              { return 1 << v;         }
    bool operator!()                    { return flags == 0;     }
    void operator += (T v)              { flags |=  flag_for(v); }
    void operator -= (T v)              { flags &= ~flag_for(v); }
    bool operator[]  (T v)              {
        uint32_t f = flag_for(v);
        return (flags & f) == f;
    }
};
///
}