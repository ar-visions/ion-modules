#include <dx/var.hpp>

// nullptr verbose, incorrect (nullptr_t is not constrained to pointers at all)
//static const nullptr_t null = nullptr;

Logging console;

int var::shape_volume(std::vector<int> &sh) {
    int sz = 1;
    for (auto d: sh)
        sz *= d;
    return sz;
}

void _log(str t, std::vector<var> a, bool error) {
#if !defined(NDEBUG)
    t = str::format(t, a);
    auto &out = error ? std::cout : std::cerr;
    out << std::string(t) << std::endl << std::flush;
#endif
}

bool var::is_numeric(const char *s) {
    return (s && (*s == '-' || isdigit(*s)));
}

std::string var::parse_numeric(char **cursor) {
    char *s = *cursor;
    if (*s != '-' && !isdigit(*s))
        return "";
    const int max_sane_number = 128;
    char*     number_start    = s;
    for (++s; ; ++s) {
        if (*s == '.' || *s == 'e' || *s == '-')
            continue;
        if (!isdigit(*s))
            break;
    }
    size_t number_len = s - number_start;
    if (number_len == 0 || number_len > max_sane_number)
       return "";
    *cursor = &number_start[number_len];
    std::string result = "";
    for (int i = 0; i < int(number_len); i++)
        result += number_start[i];
    return result;
}

/// \\ = \ ... \x = \x
std::string var::parse_quoted(char **cursor, size_t max_len) {
    const char *first = *cursor;
    if (*first != '"')
        return "";
    bool last_slash = false;
    char *start = ++(*cursor);
    char *s = start;
    for (; *s != 0; ++s) {
        if (*s == '\\')
            last_slash = true;
        else if (*s == '"' && !last_slash)
            break;
        else
            last_slash = false;
    }
    if (*s == 0)
        return "";
    size_t len = (size_t)(s - start);
    if (max_len > 0 && len > max_len)
        return "";
    *cursor = &s[1];
    std::string result = "";
    for (int i = 0; i < int(len); i++)
        result += start[i];
    return result;
}

static char ws(char **cursor) {
    char *s = *cursor;
    while (isspace(*s))
        s++;
    *cursor = s;
    if (*s == 0)
        return 0;
    return *s;
}

bool var::type_check(var &a, var &b) {
    var &aa = var::resolve(a);
    var &bb = var::resolve(b);
    return aa.t == bb.t && aa.c == bb.c;
}

var::var(nullptr_t p): var(Type::Undefined) { }

var::var(std::vector<var> data_array) : t(Type::Array) {
    size_t sz = data_array.size();
    if (sz)
        c = data_array[0].t;
    a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
    a->reserve(sz);
    for (auto &d: data_array)
        a->push_back(d);
}

bool var::operator==(uint16_t v) { return n && *((uint16_t *)n) == v; }
bool var::operator==( int16_t v) { return n && *(( int16_t *)n) == v; }
bool var::operator==(uint32_t v) { return n && *((uint32_t *)n) == v; }
bool var::operator==( int32_t v) { return n && *(( int32_t *)n) == v; }
bool var::operator==(uint64_t v) { return n && *((uint64_t *)n) == v; }
bool var::operator==( int64_t v) { return n && *(( int64_t *)n) == v; }
bool var::operator==(float    v) { return n && *((   float *)n) == v; }
bool var::operator==(double   v) { return n && *((  double *)n) == v; }
/*
var::var(FnFilter ff_) : t(Filter) {
    ff = ff_;
}
*/

var::var(Fn fn_) : t(Type::Lambda) {
    fn = fn_;
}

bool var::operator==(Type::Specifier tt) {
    return var::resolve(*this).t == tt;
}

var var::select_first(std::function<var (var &)> fn) {
    var &r = var::resolve(*this);
    if (r.a)
        for (auto &v: *r.a) {
            auto ret = fn(v); /// these can be references for identity management
            if (ret)
                return ret;
        }
    return null;
}

std::vector<var> var::select(std::function<var(var &)> fn) {
    std::vector<var> result;
    var &r = var::resolve(*this); // todo: this will likely need to be in several more places, like all..
    if (r.a)
        for (auto &v: *r.a) {
            var add = fn(v); /// these can be references for identity management
            if (add)
                result.push_back(add);
        }
    return result;
}

void var::clear() {
    var &v = var::resolve(*this);
    if (v.t == Type::Ref)
        return;
    v.n_value = {}; /// clear may just mean reset to default primitive state, so 0
    if (v.t == Type::Array)
        v.a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
    if (v.t == Type::Map)
        v.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
}

map<std::string, var> var::args(int argc, const char* argv[], Args def, std::string first_field) {
    Args        m   = def;
    std::string key = "";
    for (int i = 1; i < argc; i++) {
        auto a = argv[i];
        if (a[0] == '-' && a[1] == '-')
            key  = std::string(&a[2], strlen(&a[2]));
        if (i == 1 && first_field != "")
            m[first_field] = std::string(a);
        else if (key  != "") {
            bool  is_n = is_numeric(a);
            if (is_n)
                m[key] = stod(std::string(a)); // todo: additional conversions in var (double <-> int)
            else
                m[key] = std::string(a);
            key = "";
        }
    }
    return m;
}

map<int, int> b64_encoder() {
    auto m = ::map<int, int>();
    for (int i = 0; i < 26; i++)
        m[i] = int('A') + i;
    for (int i = 0; i < 26; i++)
        m[26 + i] = int('a') + i;
    for (int i = 0; i < 10; i++)
        m[26 * 2 + i] = int('0') + i;
    m[26 * 2 + 10 + 0] = int('+');
    m[26 * 2 + 10 + 1] = int('/');
    return m;
}

std::shared_ptr<uint8_t> var::encode(const char *data, size_t len) {
    auto m = b64_encoder();
    size_t p_len = ((len + 2) / 3) * 4;
    auto encoded = std::shared_ptr<uint8_t>(new uint8_t[p_len + 1]);
    uint8_t *e = encoded.get();
    size_t b = 0;
    for (size_t i = 0; i < len; i += 3) {
        *(e++) = m[data[i] >> 2];
        if (i + 1 <= len) *(e++) = m[((data[i]     << 4) & 0x3f) | data[i + 1] >> 4];
        if (i + 2 <= len) *(e++) = m[((data[i + 1] << 2) & 0x3f) | data[i + 2] >> 6];
        if (i + 3 <= len) *(e++) = m[  data[i + 2]       & 0x3f];
        b += std::min(size_t(4), len - i + 1);
    }
    for (size_t i = b; i < p_len; i++)
        *(e++) = '=';
    *e = 0;
    return encoded;
}

map<int, int> b64_decoder() {
    auto m = map<int, int>();
    for (int i = 0; i < 26; i++)
        m['A' + i] = i;
    for (int i = 0; i < 26; i++)
        m['a' + i] = 26 + i;
    for (int i = 0; i < 10; i++)
        m['0' + i] = 26 * 2 + i;
    m['+'] = 26 * 2 + 10 + 0;
    m['/'] = 26 * 2 + 10 + 1;
    return m;
}

std::shared_ptr<uint8_t> var::decode(const char *b64, size_t b64_len, size_t *alloc) {
    auto m = b64_decoder();
    *alloc = b64_len / 4 * 3;
    auto out = std::shared_ptr<uint8_t>(new uint8_t[*alloc + 1]);
    auto   o = out.get();
    assert(b64_len % 4 == 0);
    size_t n = 0;
    size_t e = 0;
    for (size_t ii = 0; ii < b64_len; ii += 4) {
        int a[4], w[4];
        for (int i = 0; i < 4; i++) {
            bool is_e = (a[i] == '=');
            char   ch = b64[ii + i];
            a[i]      = ch;
            w[i]      = is_e ? 0 : m[ch];
            if (a[i] == '=')
                e++;
        }
        uint8_t b0 = (w[0] << 2) | (w[1] >> 4);
        uint8_t b1 = (w[1] << 4) | (w[2] >> 2);
        uint8_t b2 = (w[2] << 6) | (w[3] >> 0);
        if (e <= 2) o[n++] = b0;
        if (e <= 1) o[n++] = b1;
        if (e <= 0) o[n++] = b2;
    }
    assert(n + e == *alloc);
    o[n] = 0;
    return out;
}

void var::reserve(size_t sz) {
    if (a)
        a->reserve(sz);
}

var::operator path_t() {
    assert(s);
    return path_t(*s);
}

var::var(path_t p, enum var::Format format) {
    assert(std::filesystem::is_regular_file(p));
    if (format == Binary) {
        std::ifstream f { p, std::ios::binary };
        *this = var { f, format };
        f.close();
    } else {
        str js = str(p);
        *this = parse_json(js);
    }
}

void var::write(path_t p, enum var::Format format) {
    std::ofstream f(p, std::ios::out | std::ios::binary);
    write(f, var::Binary);
    f.close();
}

// construct data from file ref
var::var(std::ifstream &f, enum var::Format format) {
    auto st = [&]() -> std::string {
        int len;
        f.read((char *)&len, sizeof(int));
        char *v = (char *)malloc(len + 1);
        f.read(v, len);
        v[len] = 0;
        return std::string(v, len);
    };
    
    auto rd = std::function<void(var &)>();
    rd = [&](var &d) {
        int dt;
        f.read((char *)&dt, sizeof(int));
        d.t = Type::Specifier(size_t(dt));
        
        int meta_sz = 0;
        f.read((char *)&meta_sz, sizeof(int));
        if (meta_sz)
            d.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
        for (int i = 0; i < meta_sz; i++) {
            std::string key = st();
            rd(d[key]);
        }
        if (d == Type::Array) {
            int sz;
            f.read((char *)&d.c, sizeof(int));
            f.read((char *)&sz,  sizeof(int));
            if (sz) {
                std::vector<int> sh;
                int s_sz;
                f.read((char *)&s_sz, sizeof(int));
                for (int i = 0; i < s_sz; i++) {
                    sh.push_back(0);
                    f.read((char *)&sh[sh.size() - 1], sizeof(int));
                }
                d.sh = sh;
                if (d.c >= Type::i8 && d.c <= Type::Bool) {
                    int ts = Type::size(d.c);
                    d.d = std::shared_ptr<uint8_t>(new uint8_t[ts * sz]);
                    f.read((char *)d.data<uint8_t>(), ts * sz);
                } else {
                    d.a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
                    d.a->reserve(sz);
                    for (int i = 0; i < sz; i++) {
                        d += null;
                        rd(d[i]);
                    }
                }
            }
        } else if (d == Type::Map) {
            int sz;
            f.read((char *)&sz, sizeof(int));
            for (int i = 0; i < sz; i++) {
                std::string key = st();
                rd(d[key]);
            }
        } else if (d == Type::Str) {
            int len;
            f.read((char *)&len, sizeof(int));
            d.s = std::shared_ptr<std::string>(new std::string(st()));
        } else {
            size_t ts = Type::size(d.t);
            assert(d.t >= Type::i8 && d.t <= Type::Bool);
            d.n = &d.n_value;
            f.read((char *)d.n, ts);
        }
        return d;
    };
    rd(*this);
}

void var::write(std::ofstream &f, enum var::Format format) {
    auto wri_str = std::function<void(std::string &)>();
    
    wri_str = [&](std::string &str) {
        int len = int(str.length());
        f.write((char *)&len, sizeof(int));
        f.write(str.c_str(), size_t(len));
    };
    
    auto wri = std::function<void(var &)>();
    
    wri = [&](var &d) {
        int dt = d.t;
        f.write((const char *)&dt, sizeof(int));
        size_t ts = Type::size(d.t);
        
        // meta != map in abstract; we just dont support it at runtime
        size_t meta_sz = ((d.t != Type::Map) && d.m) ? d.m->size() : 0;
        f.write((const char *)&meta_sz, sizeof(int));
        if (meta_sz)
            for (auto &[k,v]: *(d.m)) {
                std::string kname = k;
                wri_str(k);
                wri(v);
            }
        
        if (d == Type::Array) {
            int     idc = d.c;
            int      sz = d.a ? int(d.a->size()) : int(shape_volume(d.sh));
            int    s_sz = int(d.sh.size());
            f.write((const char *)&idc, sizeof(int));
            f.write((const char *)&sz, sizeof(int));
            if (sz) {
                if (d.a) {
                    int i1 = 1;
                    int sz = d.size();
                    f.write((const char *)&i1, sizeof(int));
                    f.write((const char *)&sz, sizeof(int));
                } else {
                    f.write((const char *)&s_sz, sizeof(int));
                    for (int i: d.sh)
                        f.write((const char *)&i, sizeof(int));
                }
                if (d.c >= Type::i8 && d.c <= Type::Bool) {
                    assert(d.d);
                    size_t ts = Type::size(d.c);
                    f.write((const char *)d.data<uint8_t>(), ts * size_t(sz));
                } else if (d.c == Type::Map) {
                    assert(d.a);
                    for (auto &i: *d.a)
                        wri(i);
                }
            }
        } else if (d == Type::Map) {
            assert(d.m);
            int sz = int(d.m->size());
            f.write((const char *)&sz, sizeof(int));
            for (auto &[k,v]: *(d.m)) {
                wri_str(k);
                wri(v);
            }
        } else if (d == Type::Str) {
            int len = d.s ? int(d.s->length()) : int(0);
            for (size_t i = 0; i < len; i++) {
                uint8_t v = d[i];
                f.write((const char *)&v, 1);
            }
        } else {
            assert(d.t >= Type::i8 && d.t <= Type::Bool);
            f.write((const char *)d.n, ts);
        }
    };
    
    var &v = var::resolve(*this);
    if (format == var::Binary)
        wri(v);
    else {
        auto json = str(v);
        f.write(json.cstr(), json.len());
    }
}

var::operator std::string() {
    var &r = var::resolve(*this);
    switch (size_t(r.t)) {
        case Type::i8:   [[fallthrough]];
        case Type::ui8:  return std::to_string(*((int8_t  *)r.n));
        case Type::i16:  [[fallthrough]];
        case Type::ui16: return std::to_string(*((int16_t *)r.n));
        case Type::i32:  [[fallthrough]];
        case Type::ui32: return std::to_string(*((int32_t *)r.n));
        case Type::i64:  [[fallthrough]];
        case Type::ui64: return std::to_string(*((int64_t *)r.n));
        case Type::f32:  return std::to_string(*((float   *)r.n));
        case Type::f64:  return std::to_string(*((double  *)r.n));
        case Type::Str:  return r.s ? *r.s : "";
        case Type::Array: {
            std::string ss = "[";
            if (c >= Type::i8 && c <= Type::Bool) {
                size_t  sz = shape_volume(sh);
                size_t  ts = Type::size(c);
                uint8_t *v = d.get();
                for (size_t i = 0; i < sz; i++) {
                    if (i > 0)
                        ss += ",";
                    if      (c == Type::f32)                     ss += std::to_string(*(float *)  &v[ts * i]);
                    else if (c == Type::f64)                     ss += std::to_string(*(double *) &v[ts * i]);
                    else if (c == Type::i8  || c == Type::ui8)   ss += std::to_string(*(int8_t *) &v[ts * i]);
                    else if (c == Type::i16 || c == Type::ui16)  ss += std::to_string(*(int16_t *)&v[ts * i]);
                    else if (c == Type::i32 || c == Type::ui32)  ss += std::to_string(*(int32_t *)&v[ts * i]);
                    else if (c == Type::i64 || c == Type::ui64)  ss += std::to_string(*(int64_t *)&v[ts * i]);
                    else if (c == Type::Bool)                    ss += std::to_string(*(bool *)   &v[ts * i]);
                    else
                        assert(false);
                }
            } else {
                assert(r.a); // this is fine
                for (auto &value: *r.a) {
                    if (ss.length() != 1)
                        ss += ",";
                    ss += std::string(value);
                }
            }
            ss += "]";
            return ss;
        }
        case Type::Map: {
            if (r.s)
                return *(r.s.get()); /// use-case: var-based Model names (second thoughts...)
            std::string ss = "{";
            if (r.m)
                for (auto &[field, value]: *r.m) {
                    if (ss.length() != 1)
                        ss += ",";
                    ss += std::string(field);
                    ss += ":";
                    ss += std::string(value);
                }
            ss += "}";
            return ss;
        }
        case Type::Ref: {
            if (r.n) {
                char buf[64];
                sprintf(buf, "%p", (void *)r.n);
                return std::string(buf);
            }
            return std::string(""); // null reference; context needed for null str
        }
        default:
            break;
    }
    return "";
}

var::operator bool() {
    var &v = var::resolve(*this);
    switch (size_t(v.t)) {
        case Type::Bool:  return *((  bool   *)v.n) != false;
        case Type::i8:    return *((  int8_t *)v.n) > 0;
        case Type::ui8:   return *(( uint8_t *)v.n) > 0;
        case Type::i16:   return *(( int16_t *)v.n) > 0;
        case Type::ui16:  return *((uint16_t *)v.n) > 0;
        case Type::i32:   return *(( int32_t *)v.n) > 0;
        case Type::ui32:  return *((uint32_t *)v.n) > 0;
        case Type::i64:   return *(( int64_t *)v.n) > 0;
        case Type::ui64:  return *((uint64_t *)v.n) > 0;
        case Type::f32:   return *((float    *)v.n) > 0;
        case Type::f64:   return *((double   *)v.n) > 0;
        case Type::Str:   return      v.s->length() > 0;
        case Type::Map:   return        v.m and v.m->size() > 0;
        case Type::Array: return        v.d  or v.a->size() > 0;
        case Type::Ref:   return v.n_value.vref ? bool(v.n_value.vref) : bool(v.n);
        default:
            break;
    }
    return false;
}

bool var::operator!() {
    return !(var::operator bool());
}

/// design should definitely have Type t and Type c, not Specifier
/// i am holding off on this, though
/// what it needs is basic error facilities for conversion at runtime
/// the Type really makes it truly non-duck typing on the data because we have a hash identifier
/// better to use the stricter facilities on a default basis
var::var(Type::Specifier t, Type::Specifier c, size_t sz)  : t(t == Type::Undefined ? Type::Map : t), c(
        ((t == Type::Array || t == Type::Map) && c == Type::Undefined) ? Type::Any : c) {
    if (this->t == Type::Array) {
        a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
        a->reserve(sz);
    } else if (this->t == Type::Map)
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
    else
        n = &n_value;
}

var::var(float    v) : t(Type::f32)  {
    n = &n_value;
    *((float *)n) = v;
}

var::var(double   v) : t(Type::f64)  {
    n = &n_value;
    *((double *)n) = v;
}

var::var(int8_t   v) : t(Type::i8)   {
    n = &n_value;
    *((int8_t *)n) = v;
}

var::var(uint8_t  v) : t(Type::ui8)  {
    n = &n_value;
    *((uint8_t *)n) = v;
}

var::var(int16_t  v) : t(Type::i16)  {
    n = &n_value;
    *((int16_t *)n) = v;
}

var::var(uint16_t v) : t(Type::ui16) {
    n = &n_value;
    *((uint16_t *)n) = v;
}

var::var(int32_t  v) : t(Type::i32)  {
    n = &n_value;
    *((int32_t *)n) = v;
}

var::var(uint32_t v) : t(Type::ui32) {
    n = &n_value;
    *((uint32_t *)n) = v;
}

var::var(int64_t  v, int flags) : t(Type::i64), flags(flags)  {
    n = &n_value;
    *((int64_t *)n) = v;
}

/*var::var(uint64_t v) : t(Type::ui64) {
    n = &n_value;
    *((uint64_t *)n) = v;
}*/

var::var(size_t v) : t(Type::ui64) {
    n = &n_value;
    *((uint64_t *)n) = uint64_t(v);
}

/// merge these two use cases into VoidRef, Ref, singular Ref struct (likely quite usable for data Modeling as well)
var::var(void* v)            : t(Type::Ref) { n = (u *)v;    }
var::var(VoidRef vr)         : t(Type::Ref) { n = (u *)vr.v; }
var::var(std::string str)    : t(Type::Str), s(new std::string(str)) { }
var::var(const char *str)    : t(Type::Str), s(new std::string(str)) { }
var::var(path_t p) : t(Type::Str),           s(new std::string(p.string())) { }
var::var(const var &ref) {
    copy((var &)ref);
}

char *var::parse_obj(var &scope, char *start, std::string field, FieldFlags &flags) {
    char *cur = start;
    assert(*cur == '{');
    ws(&(++cur));
    scope.t = Type::Map;
    scope.c = Type::Any;
    scope.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
    while (*cur != '}') {
        auto field = parse_quoted(&cur);
        ws(&cur);
        assert(field != "");
        assert(*cur == ':');
        ws(&(++cur));
        scope[field] = var(Type::Undefined);
        cur = parse_value(scope[field], cur, field, Type::Any, flags);
        if (ws(&cur) == ',')
            ws(&(++cur));
    }
    assert(*cur == '}');
    ws(&(++cur));
    return cur;
}

char *var::parse_arr(var &scope, char *start, std::string field, FieldFlags &flags) {
    char *cur = start;
    assert(*cur == '[');
    scope.t = Type::Array;
    scope.c = Type::Any;
    scope.a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
    ws(&(++cur));
    for (;;) {
        scope.a->push_back(var(Type::Undefined));
        cur = parse_value(scope.a->back(), cur, field, Type::Any, flags);
        ws(&cur);
        if (*cur == ',') {
            ws(&(++cur));
            continue;
        } else if (*cur == ']') {
            ws(&(++cur));
            break;
        }
        assert(false);
    }
    if (flags.count(field) > 0)
        scope.flags = flags[field].flags;
    scope.compact();
    return cur;
}

char *var::parse_value(var &scope, char *start, std::string field,
                       Type::Specifier enforce_type, FieldFlags &flags) {
    char *cur = start;
    if (*cur == '{') {
        return parse_obj(scope, start, field, flags);
    } else if (*cur == '[') {
        return parse_arr(scope, start, field, flags);
    } else if (*cur == 't' || *cur == 'f') {
        assert(enforce_type == Type::Any || enforce_type == Type::Bool);
        scope.n = &scope.n_value;
        scope.t = Type::Bool;
        *((bool *)scope.n) = *cur == 't';
        while(*cur && isalpha(*cur))
            cur++;
    } else if (*cur == '"') {
        std::string s = parse_quoted(&cur);
        if (flags.count(field) > 0 && (flags[field].flags & Flags::Encode)) {
            assert(enforce_type == Type::Any || enforce_type == Type::Array);
            size_t alloc = 0;
            scope.t  = Type::Array;
            scope.c  = flags[field].t;
            scope.d  = decode(s.c_str(), s.length(), &alloc);
            scope.sh = { int((alloc - 1) / Type::size(scope.c)) };
            assert(alloc > 0);
        } else {
            assert(enforce_type == Type::Any || enforce_type == Type::Str);
            scope.t = Type::Str;
            scope.s = std::shared_ptr<std::string>(new std::string(s));
        }
    } else if (*cur == '-' || isdigit(*cur)) {
        std::string value = parse_numeric(&cur);
        assert(value != "");
        double  d = atof(value.c_str());
        int64_t i;
        try   { i = std::stoull(value); } catch (std::exception &e) { }
        ///
        if (enforce_type == Type::Any) {
            if (flags.count(field) > 0)
                scope.t = flags[field].t;
            else if (std::floor(d) != d)
                scope.t = Type::f64;
            else
                scope.t = Type::i32;
        } else
            scope.t = enforce_type;
        ///
        if (scope.t != Type::Str) {
            scope.n = &scope.n_value;
            switch (scope.t) {
                case Type::Bool: *((    bool *)scope.n) =     bool(i); break;
                case Type::ui8:  *(( uint8_t *)scope.n) =  uint8_t(i); break;
                case Type::i8:   *((  int8_t *)scope.n) =   int8_t(i); break;
                case Type::ui16: *((uint16_t *)scope.n) = uint16_t(i); break;
                case Type::i16:  *(( int16_t *)scope.n) =  int16_t(i); break;
                case Type::ui32: *((uint32_t *)scope.n) = uint32_t(i); break;
                case Type::i32:  *(( int32_t *)scope.n) =  int32_t(i); break;
                case Type::ui64: *((uint64_t *)scope.n) = uint64_t(i); break;
                case Type::i64:  *(( int64_t *)scope.n) =  int64_t(i); break;
                case Type::f32:  *((   float *)scope.n) =    float(d); break;
                case Type::f64:  *((  double *)scope.n) =   double(d); break;
                case Type::Str:
                default:
                    assert(false);
                    break;
            }
        } else {
            scope.s = std::shared_ptr<std::string>(new std::string(value));
        }
    } else
        assert(false);
    return cur;
}

/// call this from json
var::var(Type::Specifier t, std::string str) : t(t) {
    const char *c = str.c_str();
    int64_t     i = std::strtoll(c, NULL, 10);
                n = &n_value;
    switch (size_t(t)) {
        case Type::Bool: *((    bool *)n) =     bool(*c && (*c == 't' || *c == 'T' || isdigit(*c))); break;
        case Type::ui8:  *(( uint8_t *)n) =  uint8_t(i); break;
        case Type::i8:   *((  int8_t *)n) =   int8_t(i); break;
        case Type::ui16: *((uint16_t *)n) = uint16_t(i); break;
        case Type::i16:  *(( int16_t *)n) =  int16_t(i); break;
        case Type::ui32: *((uint32_t *)n) = uint32_t(i); break;
        case Type::i32:  *(( int32_t *)n) =  int32_t(i); break;
        case Type::ui64: *((uint64_t *)n) = uint64_t(i); break;
        case Type::i64:  *(( int64_t *)n) =  int64_t(i); break;
        case Type::f32:  *((   float *)n) =    float(std::stod(str)); break;
        case Type::f64:  *((  double *)n) =   double(std::stod(str)); break;
        case Type::Str: {
            n = null;
            s = std::shared_ptr<std::string>(new std::string(str));
            break;
        }
        default: {
            /// invalid general usage at the moment
            assert(false);
            break;
        }
    }
    
}

var var::parse_json(str &js) {
    var r;
    FieldFlags flags;
    ///
    parse_value(r, (char *)js.cstr(), "", Type::Any, flags);
    return r;
}

var var::load(path_t p) {
    if (!std::filesystem::is_regular_file(p))
        return null;
    str js = str::read_file(p);
    return parse_json(js);
}

var &var::operator[](str s) {
    str ss = s;
    var &v = var::resolve(*this);
    if (!v.m)
         v.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>);
    return (*v.m)[s];
}

void var::compact() {
    if (!a || t != Type::Array || (flags & Flags::Compact))
        return;
    flags    |= Flags::Compact;
    int type  = -1;
    int types =  0;
    for (auto &i: *a)
        if (i.t != type) {
            type = i.t;
            types++;
        }
    if (types == 1) {
        size_t ts = Type::size(Type::Specifier(size_t(type)));
        if (ts == 0)
            return;
        size_t sz = a->size();
        var   *r = a->data();
        assert(type >= Type::i8 && type <= Type::Bool);
        sh = { int(sz) };
        d  = std::shared_ptr<uint8_t>((uint8_t *)malloc(ts * sz));
        
        switch (t) {
            case Type::Bool: {
                bool *v = (bool *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((bool *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::i8:
            case Type::ui8: {
                int8_t *v = (int8_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int8_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::i16:
            case Type::ui16: {
                int16_t *v = (int16_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int16_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::i32:
            case Type::ui32: {
                int32_t *v = (int32_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int32_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::i64:
            case Type::ui64: {
                int64_t *v = (int64_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int64_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::f32: {
                float *v = (float *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((float *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case Type::f64: {
                double *v = (double *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((double *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            default:
                break;
        }
        

    } else
        assert(false);
}

// resolve from vrefs potentially

var &var::resolve(var &in) {
    var *p = &in;
    while (p && p->t == Type::Ref)
        p = p->n_value.vref;
    return *p;
}

void var::notify_update() {
    var &v = resolve(*this);
    if (!v.observer)
        return;
    if (v.row) {
        /// [design] row is not set on new rows, because it has not yet been added.
        assert(v.t != Type::Ref);
        assert(v.field != "");
        str   s_field = v.field;
        observer->fn_change(Binding::Update, *v.row, s_field, v);
    }
}

void var::notify_insert() {
    var &v = resolve(*this);
    if (!v.observer)
        return;
    assert( v.t != Type::Ref);
    assert(!v.row);
    str snull = null;
    var vnull = null; /// print observer values; they should all point to the same var
    observer->fn_change(Binding::Insert, v, snull, vnull);
}

void var::notify_delete() {
    var &v = resolve(*this);
    if (!v.observer)
        return;
    assert( v.t != Type::Ref);
    assert(!v.row);
    str snull = null;
    var vnull = null;
    observer->fn_change(Binding::Delete, v, snull, vnull);
}

var::~var() {
    if (flags & DestructAttached)
        fn(*this); // arb value
}

var:: var(str s) : t(Type::Str), s(new std::string(s.s)) { }

std::vector<int> var::shape() {
    var &v = resolve(*this);
    if (v.sh.size())
        return v.sh;
    return std::vector<int> { v.size() ? int(v.size()) : int(Type::size(v.t)) };
}

void var::set_shape(std::vector<int> s) {
    var &v = resolve(*this);
    v.sh = s;
}

var var::tag(::map<std::string, var> m) {
    var v = copy();
    assert(v.t != Type::Map);
    v.m    = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
    for (auto &[field, value]: *v.m)
        v[field] = value;
    return v;
}

size_t var::size() {
    var &v = var::resolve(*this);
    int sz = 1;
    for (auto d: v.sh)
        sz *= d;
    if (v.t == Type::Array)
        return v.d ? sz : (v.a ? v.a->size() : 0);
    else if (v.t == Type::Map)
        return m->size();
    else if (v.t == Type::Str)
        return v.s ? v.s->length() : 0;
    return v.t == Type::Undefined ? 0 : 1;
}

void var::each(FnEach each) {
    var &v = var::resolve(*this);
    if (v.m)
        for(auto &[field, value]: *v.m)
            each(value, field);
}

void var::each(FnArrayEach each) {
    // never do a null check here, never.  do not cater to missing data
    size_t i = 0;
    if (a)
        for(auto &d: *a)
            each(d, i++);
}

/// once we get an assign= operator, you are notified with the value, and you call update
void var::observe_row(var &row) {
    var &o = var::resolve(*this);
    var &r = var::resolve(row);
    r.observer = o.observer ? o.observer : o.ptr();
    for (auto &[field, value]: r.map()) {
        var &v = var::resolve(value);
        v.observer = r.observer;
        v.field    = field;
        v.row      = r.ptr();
    }
}

/// once we get an assign= operator, you are notified with the value, and you call update
void var::observe(std::function<void(Binding op, var &row, str &field, var &value)> fn) {
    var &v = var::resolve(*this);
    assert(v.t == Type::Array);
    for (auto &row: *v.a)
        v.observe_row(row);
    v.observer  = v.ptr(); /// bug fix: i think that was a boolean op again.
    v.fn_change = fn;
}

/// external copy; refs as resolved in this process
var var::copy() {
    var &v = var::resolve(*this);
    if (v.t == Type::Map) {
        var res = { v.t, v.c };
        for (auto &[field, value]: v.map())
            res[field] = value;
        return res;
    } else if (v.t == Type::Array) {
        var res = { v.t, v.c };
        if (a) for (auto &e: *v.a) /// these will be hard to spot if missing v.*
            res += e;
        return res;
    } else
        return *this;
}

/// interal copy; refs copy as refs; this is part of a normal copy process
void var::copy(var &ref) {
    t        = ref.t;
    c        = ref.c;
    d        = ref.d;
    a        = ref.a;
    s        = ref.s;
    m        = ref.m;
    sh       = ref.sh;
    flags   |= ref.flags; /// accumulate flags.
    n_value  = ref.n_value;
    if (ref.observer) { /// todo: pack into observer delegate, smart ptr user, refrain from any allocation unless set
        observer  = ref.observer;
        field     = ref.field;
        row       = ref.row;
        fn_change = ref.fn_change;
    }
    //if (ref.ff)
    //    ff = ref.ff;
    if (ref.fn)
        fn = ref.fn;
    
    if (ref.n == &ref.n_value)
        n = &n_value;
    else if (ref.t == Type::Ref) /// Important to remember the public t member, if we know its a reference and this is a copy constructor.
        n = ref.n;
    else
        n = ref.n;
}

void var::operator+= (var ref) {
    var  &v = var::resolve(*this);
    auto &a = *v.a;
    a.push_back(ref);
    if (v.observer) {
        var &e = a[a.size() - 1];
        v.observe_row(e);  /// update bindings
        e.notify_insert(); ///
    }
}

var& var::operator = (const var &ref) {
    /// [design] no resolves on assignment
    if (this != &ref)
        copy((var &)ref);
    
    /// post-copy
    if (!(flags & ReadOnly))
        notify_update();
    return *this;
}

bool var::operator!= (const var &ref) {
    var  &v = var::resolve(*this);
    if (v.t != ref.t)
        return true;
    else if (v == Type::Lambda) {
        
    } else if (v.t == Type::Undefined || v.t == Type::Any)
        return false;
    else if (v.t == Type::Str)
        return *v.s != *(ref.s);
    else if (v.t == Type::Map) {
        if (v.m ? v.m->size() : 0 != ref.m ? ref.m->size() : 0)
            return true;
        if (v.m)
            for (auto &[field, value]: *v.m) {
                if (ref.m->count(field) == 0)
                    return true;
                auto &rd = (*(ref.m))[field];
                if (value != rd)
                    return true;
            }
        return true;
    } else if (v.t == Type::Array) {
        size_t a_sz =   v.a ?   v.a->size() : 0;
        size_t b_sz = ref.a ? ref.a->size() : 0;
        if (a_sz != b_sz)
            return true;
        size_t i = 0;
        if (v.a)
            for (auto &value: *v.a) {
                auto &rd = (*(ref.a))[i++];
                if (value != rd)
                    return true;
            }
    } else if (v.t == Type::Ref) {
        assert(false); // todo
    } else {
        assert(v.t >= Type::i8 && v.t <= Type::Bool);
        size_t  ts = Type::size(v.t);
        bool match = memcmp(v.n, ref.n, ts) == 0;
        return !match;
    }
    return true;
}

bool var::operator==(::map<std::string, var> &map) {
    var &v = var::resolve(*this);
    if (!v.m)
        return map.size() == 0;
    return map == *v.m;
}

bool var::operator!=(::map<std::string, var> &map) {
    return !operator==(map);
}

bool var::operator==(const var &ref) {
    return !(operator!=(ref));
}

bool var::operator==(var &ref) {
    return !(operator!=((const var &)ref));
}

bool exists(path_t &p) {
    return std::filesystem::exists(p);
}


void resources(path_t p, std::vector<const char *> exts, std::function<void(path_t &)> fn) {
    if (std::filesystem::is_directory(p)) {
        for (const auto &f: std::filesystem::directory_iterator(p)) {
            path_t path = f.path();
            if (!f.is_regular_file() || !exists(path))
                continue;
            std::string ext = f.path().extension().string();
            for (size_t i = 0; exts.size() == 0 || i < exts.size(); i++)
                if (exts.size() == 0 || exts[i] == ext) {
                    fn(path);
                    break;
                }
        }
    } else if (exists(p))
        fn(p);
}


    
