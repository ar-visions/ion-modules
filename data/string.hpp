#pragma once
#include <data/data.hpp>
#include <data/array.hpp>

struct str {
    std::string s;
    str(nullptr_t n = null);
    str(const char *cstr);
    str(const char *cstr, size_t len);
    str(std::string s);
    str(std::ifstream& in);
    str(path_t p);
    str(char c);
    size_t find(str &str, size_t from) const;
    size_t length() const;
    str replace(str from, str to) const;
    str replace(const char *from, const char *to) const;
    str substr(size_t start, size_t len) const;
    str substr(size_t start) const;
    int index_of(const char *f) const;
    vec<str> split(str delim) const;
    vec<str> split(const char *delim) const;
    static str format(str t, std::vector<var> p = {});
    /*
    str operator %(std::vector<var> p) {
        return format(str(s), p);
    }*/
    
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
    str operator+   (const str &b) { return str(s + b.s);    }
    str &operator+= (const str &b) { s += b.s; return *this; }
};

typedef vec<str> Strings;

namespace std {
    template<> struct hash<str> {
        size_t operator()(str const& s) const { return hash<std::string>()(s.s); }
    };
}
