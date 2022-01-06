#include <dx/var.hpp>
#include <fstream>

str::str(nullptr_t n)                                          { }
str::str(std::string s)    : s(s)                              { }
str::str(const char *s)    : s(s)                              { }
str::str(const char *s, size_t len) : s({s, len})              { }

str::str(vec<char> &v) {
    s = std::string((const char *)v.data(), v.size());
}
str::operator path_t() const {
    return s;
}

str str::read_file(path_t f) {
    if (!std::filesystem::is_regular_file(f))
        return null;
    std::ifstream fs;
    fs.open(f);
    std::ostringstream sstr;
    sstr << fs.rdbuf();
    fs.close();
    return str(sstr.str());
}

bool str::starts_with(const char *cstr) const {
    size_t l0 = strlen(cstr);
    size_t l1 = len();
    if (l1 < l0)
        return false;
    return memcmp(cstr, s.c_str(), l0) == 0;
}

bool str::ends_with(const char *cstr) const {
    size_t l0 = strlen(cstr);
    size_t l1 = len();
    if (l1 < l0)
        return false;
    const char *end = &(s.c_str())[l1 - l0];
    return memcmp(cstr, end, l0) == 0;
}

str   str::operator()(size_t start, size_t len)  const {
    return substr(start, len);
}
str   str::operator()(size_t start)              const {
    return substr(start);
}

str::str(int64_t v) {
    s = std::to_string(v);
}

str::str(char c) {
    char cv[2] = { c, 0 };
    s = std::string(cv);
}

str::str(int v) {
    s = std::to_string(v);
}

str::str(std::ifstream& in) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    s = sstr.str();
}

/*
bool str::operator< (const str& rhs) const {
    return s < rhs.s;
}
*/

str str::to_lower() const {
    std::string s = this->s;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); // transform and this style of api is more C than C++
    return s;
}

str str::to_upper() const {
    std::string s = this->s;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

size_t str::find(str &str, size_t from) const {
    return s.find(str.s, from);
}

size_t str::len() const {
    return s.length();
}

const char *str::cstr() const {
    return s.c_str();
}

str str::replace(str from, str to, bool all) const {
    size_t start_pos = 0;
    std::string str = s;
    while((start_pos = str.find(from.s, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.len(), to.s);
        start_pos += to.len();
        if (!all)
            break;
    }
    return str;
}

str str::substr(int start, size_t len) const {
    return start >= 0 ? s.substr(size_t(start), len) : s.substr(size_t(std::max(0, int(s.length()) + start)), len);
}

str str::substr(int start) const {
    return start >= 0 ? s.substr(size_t(start)) : s.substr(size_t(std::max(0, int(s.length()) + start)));
}
/*
str::operator var() {
    return s;
}*/

str::str(var &d) : s(std::string(d)) { }

vec<str> str::split(str delim) const {
    size_t start = 0, end, delim_len = delim.len();
    ::vec<str> result;
    if (s.length() > 0) {
        while ((end = s.find(delim.s, start)) != std::string::npos) {
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

vec<str> str::split(const char *delim) const {
    return split(std::string(delim));
}

/// a common split, this splits by white space and comma
vec<str> str::split() const {
    ::vec<str> result;
    str chars = "";
    for (char const &c: s) { /// replace traditional for uses where index not used
        bool is_ws = isspace(c) || c == ',';
        if (is_ws) {
            if (chars) {
                result += chars;
                chars = "";
            }
        } else
            chars += str(c);
    }
    if (chars || !result)
        result += chars;
    return result;
}

int str::index_of(const char *f) const {
    std::string::size_type loc = s.find(f, 0);
    if (loc != std::string::npos)
        return int(loc);
    return -1;
}

int str::index_icase(const char *f) const {
    const std::string fs = f;
    auto it = std::search(s.begin(), s.end(), fs.begin(), fs.end(), [](char ch1, char ch2) {
        return std::toupper(ch1) == std::toupper(ch2);
    });
    return (it == s.end()) ? -1 : it - s.begin();
}

str::operator std::string() const & {
    return s;
}

str::operator bool() const {
    return s != "";
}

bool str::operator!() const {
    return s == "";
}

bool str::operator==(const char *cstr) const {
    return strcmp(s.c_str(), cstr) == 0;
}

bool str::operator!=(const char *cstr) const {
    return strcmp(s.c_str(), cstr) != 0;
}

bool str::operator==(std::string str) const {
    return s == str;
}

bool str::operator!=(std::string str) const {
    return s != str;
}

const char &str::operator[](size_t i) const {
    return s[i];
}

int str::integer() const {
    const char *c = s.c_str();
    while (isalpha(*c))
        c++;
    return s.length() ? int(strtol(c, null, 10)) : 0;
}

double str::real() const {
    const char *c = s.c_str();
    while (isalpha(*c))
        c++;
    return strtod(c, null);
}

bool str::is_numeric() const {
    return s != "" && (s[0] == '-' || isdigit(s[0]));
}

str str::format(str t, std::vector<var> p) {
    for (size_t k = 0; k < p.size(); k++) {
        auto p0 = str("{") + str(std::to_string(k)) + str("}");
        auto p1 = str(p[k]);
        t       = t.replace(p0, p1);
    }
    return t;
}

/// trim white space
str str::trim() const {
    auto   start  = s.begin();
    while (start != s.end() && std::isspace(*start))
           start++;
    auto end = s.end();
    do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

