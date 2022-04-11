export module dx:string;
export {

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

import dx:size;

struct string {
protected:
    typedef std::shared_ptr<std::string> shared;
    shared sh;
    std::string &ref() const { return *sh; }
    
public:
    enum MatchType {
        Alpha,
        Numeric,
        WS,
        Printable,
        String,
        CIString
    };
    
    /// constructors
    string()                                   : sh(shared(new std::string())) { }
    string(nullptr_t n)                        : sh(shared(new std::string())) { }
    string(const char *cstr)                   : sh(shared(cstr ? new std::string(cstr)      : new std::string())) { }
    string(cchar_t *cstr, Size len)            : sh(shared(cstr ? new std::string(cstr, len) : new std::string())) {
        if (!cstr && len > 0) sh->reserve(len);
    }
  //string(const char      ch)                 : sh(shared(new std::string(&ch, 1)))              { }
    string(Size            sz)                 : sh(shared(new std::string()))                    { sh->reserve(sz); }
    string(int32_t        i32)                 : sh(shared(new std::string(std::to_string(i32)))) { }
    string(int64_t        i64)                 : sh(shared(new std::string(std::to_string(i64)))) { }
    string(double         f64)                 : sh(shared(new std::string(std::to_string(f64)))) { }
    string(std::string    str)                 : sh(shared(new std::string(str)))                 { }
    string(const string  &s)                   : sh(s.sh)                                         { }
   ~string() { }
    
    /// methods
    const char *cstr()                           const { return ref().c_str();                  }
    bool        contains(array<string>   a)      const { return index_of_first(a, null) >= 0;   }
    Size        size()                           const { return ref().length();                 }
    
    /// operators
    string      operator+  (const string    &s)  const { return ref() + s.ref();                }
    string      operator+  (const char      *s)  const { return ref() + s;                      }
    string      operator() (size_t s, size_t l)  const { return substr(s, l);                   }
    string      operator() (size_t           s)  const { return substr(s);                      }
    bool        operator<  (const string&  rhs)  const { return ref() < rhs.ref();              }
    bool        operator>  (const string&  rhs)  const { return ref() > rhs.ref();              }
    bool        operator== (const string&  b)    const { return sh == b.sh || ref() == b.ref(); }
    bool        operator!= (const char   *cstr)  const { return !operator==(cstr);              }
    bool        operator== (std::string    str)  const { return str == ref();                   }
    bool        operator!= (std::string    str)  const { return str != ref();                   }
    int         operator[] (string           s)  const { return index_of(s.cstr());             }
    int         operator[] (array<string>    a)  const { return index_of_first(a, null);        }
    void        operator+= (string           s)        { ref() += s.ref();                      }
    void        operator+= (const char   *cstr)        { ref() += cstr;                         }
    void        operator+= (const char      ch)        { ref().push_back(ch);                   }
    const char &operator[] (size_t       index)  const { return ref()[index];                   }
                operator std::filesystem::path() const { return ref();                          }
                operator std::string &()         const { return ref();                          }
                operator bool()                  const { return  sh && *sh != "";               }
    bool        operator!()                      const { return !(operator bool());             }
                operator int()                   const { return integer();                      }
                operator real()                  const { return real();                         }
              //operator const char *()          const { return ref().c_str(); } -- shes not safe to fly.
    string     &operator= (const string &s)            { if (this != &s) sh = s.sh; return *this; }
    friend std::ostream &operator<< (std::ostream& os, string const& m) { return os << m.ref(); }

    ///
    static string fill(Size n, std::function<char(size_t)> fn) {
        auto ret = string(n);
        for (size_t i = 0; i < n; i++)
            ret += fn(i);
        return ret;
    }

    ///
    static string read_file(std::filesystem::path f) {
        if (!std::filesystem::is_regular_file(f))
            return null;
        std::ifstream fs;
        fs.open(f);
        std::ostringstream sstr;
        sstr << fs.rdbuf();
        fs.close();
        return string(sstr.str());
    }

    /// a string of lambda results
    static string of(int count, std::function<char(size_t)> fn) {
        string s;
        std::string &str = s.ref();
        for (size_t i = 0; i < count; i++)
            str += fn(i);
        return s;
    }

    int index_of_first(array<string> &a, int *a_index) const {
        int less  = -1;
        int index = -1;
        for (string &ai:a) {
              ++index;
            int i = index_of(ai);
            if (i >= 0 && (less == -1 || i < less)) {
                less = i;
                if (a_index)
                   *a_index = index;
            }
        }
        if  (a_index)
            *a_index = -1;
        return less;
    }

    ///
    bool starts_with(const char *cstr) const {
        size_t l0 = strlen(cstr);
        size_t l1 = size();
        if (l1 < l0)
            return false;
        return memcmp(cstr, sh->c_str(), l0) == 0;
    }

    ///
    bool ends_with(const char *cstr) const {
        size_t l0 = strlen(cstr);
        size_t l1 = size();
        if (l1 < l0)
            return false;
        const char *end = &(sh->c_str())[l1 - l0];
        return memcmp(cstr, end, l0) == 0;
    }

    ///
    static string read_file(std::ifstream& in) {
        string s;
        std::ostringstream sstr;
        sstr << in.rdbuf();
        s = sstr.str();
        return s;
    }
    
    ///
    string to_lower() const {
        std::string c = ref();
        std::transform(c.begin(), c.end(), c.begin(), ::tolower); // transform and this style of api is more C than C++
        return c;
    }

    ///
    string to_upper() const {
        std::string c = ref();
        std::transform(c.begin(), c.end(), c.begin(), ::toupper);
        return c;
    }
    
    static int nib_value(cchar_t c) {
        return (c >= '0' and c <= '9') ? (     c - '0') :
               (c >= 'a' and c <= 'f') ? (10 + c - 'a') :
               (c >= 'A' and c <= 'F') ? (10 + c - 'A') : -1;
    }
    
    static cchar_t char_from_nibs(cchar_t c1, cchar_t c2) {
        int nv0 = nib_value(c1);
        int nv1 = nib_value(c2);
        return (nv0 == -1 || nv1 == -1) ? ' ' : ((nv0 * 16) + nv1);
    }

    string replace(string sfrom, string sto, bool all = true) const {
        size_t start_pos = 0;
        std::string  str = *sh;
        std::string  &fr = *sfrom.sh;
        std::string  &to = *sto.sh;
        while((start_pos = str.find(fr, start_pos)) != std::string::npos) {
            str.replace(start_pos, fr.length(), to);
            start_pos += to.length();
            if (!all)
                break;
        }
        return str;
    }

    string substr(int start, size_t len) const {
        std::string &s = ref();
        return start >= 0 ? s.substr(size_t(start), len) :
               s.substr(size_t(std::max(0, int(s.length()) + start)), len);
    }

    string substr(int start) const {
        std::string &s = ref();
        return start >= 0 ? s.substr(size_t(start)) :
               s.substr(size_t(std::max(0, int(s.length()) + start)));
    }

    array<string> split(string delim) const {
        array<string> result;
        size_t        start = 0, end, delim_len = delim.size();
        std::string  &s     = ref();
        std::string  &ds    = delim.ref();
        ///
        if (s.length() > 0) {
            while ((end = s.find(ds, start)) != std::string::npos) {
                std::string token = s.substr(start, end - start);
                start = end + delim_len;
                result += token;
            }
            auto token = s.substr(start);
            result += token;
        } else
            result += std::string("");
        return result;
    }

    array<string> split(const char *delim) const {
        return split(string(delim));
    }
    
    array<string> split() const {
        array<string> result;
        string chars = "";
        for (const char &c: ref()) {
            bool is_ws = isspace(c) || c == ',';
            if (is_ws) {
                if (chars) {
                    result += chars;
                    chars   = "";
                }
            } else
                chars += c;
        }
        if (chars || !result)
            result += chars;
        return result;
    }

    int index_of(const char *f) const {
        std::string::size_type loc = ref().find(f, 0);
        return (loc != std::string::npos) ? int(loc) : -1;
    }
    
    int index_of(string s)     const { return index_of(s.cstr()); }
    int index_of(MatchType ct, const char *mp = null) const {
        typedef std::function<bool(const char &)>    Fn;
        typedef std::unordered_map<MatchType, Fn>    Map;
        ///
        int index = 0;
        char *mp0 = (char *)mp;
        if (ct == CIString) {
            size_t sz = size();
            mp0       = new char[sz + 1];
            memcpy(mp0, cstr(), sz);
            mp0[sz]   = 0;
        }
        auto m = Map {
            { Alpha,     [ ](const char &c) { return  isalpha (c);            }},
            { Numeric,   [ ](const char &c) { return  isdigit (c);            }},
            { WS,        [ ](const char &c) { return  isspace (c);            }},
            { Printable, [ ](const char &c) { return !isspace (c);            }},
            { String,    [&](const char &c) { return  strcmp  (&c, mp0) == 0; }},
            { CIString,  [&](const char &c) { return  strcmp  (&c, mp0) == 0; }}
        };
        Fn &fn = m[ct];
        for (const char c:ref()) {
            if (fn(c))
                return index;
            index++;
        }
        if (ct == CIString)
            delete mp0;
        
        return -1;
    }

    int index_icase(const char *f) const {
        const std::string fs = f;
        auto &s = ref();
        auto it = std::search(s.begin(), s.end(), fs.begin(), fs.end(),
            [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
        return (it == s.end()) ? -1 : it - s.begin();
    }

    bool operator==(const char *cstr) const {
        auto &s = ref();
        if (!cstr)
            return s.length() == 0;
        const char *s0 = s.c_str();
        for (size_t i = 0; ;i++)
            if (s0[i] != cstr[i])
                return false;
            else if (s0[i] == 0 || cstr[i] == 0)
                break;
        return true;
    }

    int integer() const {
        auto &s = ref();
        const char *c = s.c_str();
        while (isalpha(*c))
            c++;
        return s.length() ? int(strtol(c, null, 10)) : 0;
    }

    double real() const {
        auto &s = ref();
        const char *c = s.c_str();
        while (isalpha(*c))
            c++;
        return strtod(c, null);
    }

    bool has_prefix(string i) const {
        auto &s = ref();
        int isz = i.size();
        int  sz =   size();
        return sz >= isz ? strncmp(s.c_str(), i.cstr(), isz) == 0 : false;
    }

    bool numeric() const {
        auto &s = ref();
        return s != "" && (s[0] == '-' || isdigit(s[0]));
    }
    
    /// wildcard match
    bool matches(string pattern) const {
        auto &s = ref();
        std::function<bool(char *, char *)> s_cmp;
        ///
        s_cmp = [&](char *str, char *pat) -> bool {
            if (!*pat)
                return true;
            if (*pat == '*')
                return (*str && s_cmp(str + 1, pat)) ? true : s_cmp(str, pat + 1);
            return (*pat == *str) ? s_cmp(str + 1, pat + 1) : false;
        };
        ///
        for (char *str = (char *)s.c_str(), *pat = (char *)pattern.cstr(); *str != 0; str++)
            if (s_cmp(str, pat))
                return true;
        ///
        return false;
    }

    string trim() const {
        auto       &s = ref();
        auto   start  = s.begin();
        while (start != s.end() && std::isspace(*start))
               start++;
        auto end = s.end();
        do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
        return std::string(start, end + 1);
    }
};

static inline string operator+(const char *cstr, string rhs) { return string(cstr) + rhs; }

typedef string str;

typedef array<str> Strings;

namespace std {
    template<> struct hash<str> {
        size_t operator()(str const& s) const { return hash<std::string>()(std::string(s)); }
    };
}

namespace std {
    template<> struct hash<path_t> {
        size_t operator()(path_t const& p) const { return hash<std::string>()(p.string()); }
    };
}

template<typename>
struct is_strable                 : std::false_type {};
template<> struct is_strable<str> : std::true_type  {};

///
struct Symbol {
    int      value;
    string   symbol;
    bool operator==(int v) { return v == value; }
};

typedef array<Symbol> Symbols;
///
}
