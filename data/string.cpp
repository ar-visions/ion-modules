#include <data/data.hpp>

str::str(nullptr_t n)                                          { }
str::str(std::string s)    : s(s)                              { }
str::str(const char *s)    : s(s)                              { }
str::str(const char *s, size_t len) : s({s, len})              { }

str::str(char c) {
    char cv[2] = { c, 0 };
    s = std::string(cv);
}

str::str(std::ifstream& in) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    s = sstr.str();
}

str::str(std::filesystem::path p) {
    std::ifstream f(p);
    std::ostringstream sstr;
    sstr << f.rdbuf();
    s = sstr.str();
}
/*
bool str::operator< (const str& rhs) const {
    return s < rhs.s;
}
*/

str str::to_lower() const {
    std::string s = this->s;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); // transform is more C than C++
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

size_t str::length() const {
    return s.length();
}

const char *str::cstr() const {
    return s.c_str();
}

str str::replace(str from, str to) const {
    size_t start_pos = 0;
    std::string str = s;
    while((start_pos = str.find(from.s, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to.s);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

str str::substr(size_t start, size_t len) const {
    return s.substr(start, len);
}

str str::substr(size_t start) const {
    return s.substr(start);
}
/*
str::operator Data() {
    return s;
}*/

str::str(Data &d) : s(std::string(d)) { }

vec<str> str::split(str delim) const {
    size_t start = 0, end, delim_len = delim.length();
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

int str::index_of(const char *f) const {
    std::string::size_type loc = s.find(f, 0);
    if (loc != std::string::npos)
        return int(loc);
    return -1;
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
    return s == cstr;
}

bool str::operator!=(const char *cstr) const {
    return s != cstr;
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
    return s.length() ? std::stoi(s) : 0;
}

double str::real() const {
    return std::stod(s);
}

bool str::is_numeric() const {
    return s != "" && (s[0] == '-' || isdigit(s[0]));
}

str str::format(str t, std::vector<Data> p) {
    for (size_t k = 0; k < p.size(); k++) {
        auto p0 = str("{") + str(std::to_string(k)) + str("}");
        auto p1 = str(p[k]);
        t       = t.replace(p0, p1);
    }
    return t;
}


