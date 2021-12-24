#pragma once
#include <dx/var.hpp>
#include <dx/array.hpp>

/// io is basically just decoration.  dont tell that to its face, though.
struct str:io {
    std::string s;
    str(nullptr_t n = null);
    str(const char *cstr);
    str(const char *cstr, size_t len);
    str(vec<char> &v);
    str(std::string s);
    str(std::ifstream& in);
    str(path_t p);
    str(char c);
    str(int  v); // deprecate for ambiguity sake
    str(int64_t v);
    size_t      find(str &str, size_t from)     const;
    size_t    length()                          const;
    static str read_file(path_t p);
    str      replace(str, str, bool all = true) const;
    str       substr(size_t start, size_t len)  const;
    str       substr(size_t start)              const;
    str   operator()(size_t start, size_t len)  const;
    str   operator()(size_t start)              const;
    bool starts_with(const char *cstr)          const;
    bool   ends_with(const char *cstr)          const;
    int     index_of(const char *f)             const;
    vec<str>   split(str delim)                 const;
    vec<str>   split(const char *delim)         const;
    vec<str>   split()                          const;
    /// not definite on the decision of str vs const char *
    static str format(str t, std::vector<var> p = {});
    
    friend auto operator<<(std::ostream& os, str const& m) -> std::ostream& {
        return os << m.s;
    }
    
    const char *cstr() const;
    
    operator std::string()                const &;
    operator bool()                       const;
    bool   operator!()                    const;
    bool   operator==(const char *cstr)   const;
    bool   operator!=(const char *cstr)   const;
    bool   operator==(std::string str)    const;
    bool   operator!=(std::string str)    const;
    const  char &operator[](size_t i)     const;
    bool   operator< (const str& rhs)     const {
        return s < rhs.s;
    }
    operator path_t()      const;
    int    integer()                      const;
    double real()                         const;
    bool   is_numeric()                   const;
  //operator var();
    str(var &d);
    str to_lower()                        const;
    str to_upper()                        const;
    str operator+   (const str  &b) { return str(s + b.s); }
    str operator+   (const char *b) { return str(s + std::string(b)); }
    str &operator+= (const str  &b) { s += b.s; return *this; }
};

typedef vec<str> Strings;

namespace std {
    template<> struct hash<str> {
        size_t operator()(str const& s) const { return hash<std::string>()(s.s); }
    };
}
