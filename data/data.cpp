#include <data/data.hpp>

Logging console;


int var::shape_volume(std::vector<int> &sh) {
    int sz = 1;
    for (auto d: sh)
        sz *= d;
    return sz;
}

void _log(str t, std::vector<var> a, bool error) {
#ifndef NDEBUG
    t = str::format(t, a);
    auto &out = error ? std::cout : std::cerr;
    out << std::string(t) << std::endl << std::flush;
#endif
}

static bool is_numeric(const char *s) {
    return (s && (*s == '-' || isdigit(*s)));
}

static std::string parse_numeric(char **cursor) {
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

static std::string parse_quoted(char **cursor, size_t max_len = 0) {
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
/*
static std::string parse_symbol(const char **cursor) {
    const int max_sane_symbol = 128;
    const char *sym_start = *cursor;
    const char *s = *cursor;
    while (isalpha(*s))
        s++;
    *cursor = s;
    size_t sym_len = s - sym_start;
    if (sym_len == 0 || sym_len > max_sane_symbol)
        return "";
    std::string result = "";
    for (size_t i = 0; i < sym_len; i++)
        result += sym_start[i];
    return result;
}*/

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
    return a.t == b.t && a.c == b.c;
}

var::var(nullptr_t p): var(Undefined) { }

var::var(std::vector<var> data_array) : t(Array) {
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

var::var(FnFilter ff_) : t(Filter) {
    ff = ff_;
}

var::var(Fn fn_) : t(Lambda) {
    fn = fn_;
}

Args var::args(int argc, const char* argv[], Args def) {
    Args m = def;
    std::string key = "";
    for (int i = 1; i < argc; i++) {
        auto a = argv[i];
        if (a[0] == '-' && a[1] == '-')
            key = std::string(&a[2], strlen(&a[2]));
        else if (key != "") {
            bool is_n = is_numeric(a);
            if (is_n)
                m[key] = stod(std::string(a)); // todo: additional conversions in var (double <-> int)
            else
                m[key] = std::string(a);
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
    *e = NULL;
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
    o[n] = NULL;
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
    std::ifstream f { p, std::ios::binary };
    *this = var { f, format };
    f.close();
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
        d.t = Type(dt);
        
        int meta_sz = 0;
        f.read((char *)&meta_sz, sizeof(int));
        if (meta_sz)
            d.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
        for (int i = 0; i < meta_sz; i++) {
            std::string key = st();
            rd(d[key]);
        }
        if (d.t == Array) {
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
                if (d.c >= i8 && d.c <= Bool) {
                    int ts = type_size(d.c);
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
        } else if (d.t == Map) {
            int sz;
            f.read((char *)&sz, sizeof(int));
            for (int i = 0; i < sz; i++) {
                std::string key = st();
                rd(d[key]);
            }
        } else if (d.t == Str) {
            int len;
            f.read((char *)&len, sizeof(int));
            d.s = std::shared_ptr<std::string>(new std::string(st()));
        } else {
            size_t ts = type_size(d.t);
            assert(d.t >= i8 && d.t <= Bool);
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
        size_t ts = type_size(d.t);
        
        // meta != map in abstract; we just dont support it at runtime
        size_t meta_sz = ((d.t != Map) && d.m) ? d.m->size() : 0;
        f.write((const char *)&meta_sz, sizeof(int));
        if (meta_sz)
            for (auto &[k,v]: *(d.m)) {
                std::string kname = k;
                wri_str(k);
                wri(v);
            }
        
        if (d.t == Array) {
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
                if (d.c >= i8 && d.c <= Bool) {
                    assert(d.d);
                    size_t ts = type_size(d.c);
                    f.write((const char *)d.data<uint8_t>(), ts * size_t(sz));
                } else if (d.c == var::Map) {
                    assert(d.a);
                    for (auto &i: *d.a)
                        wri(i);
                }
            }
        } else if (d.t == Map) {
            assert(d.m);
            int sz = int(d.m->size());
            f.write((const char *)&sz, sizeof(int));
            for (auto &[k,v]: *(d.m)) {
                wri_str(k);
                wri(v);
            }
        } else if (d.t == Str) {
            int len = d.s ? int(d.s->length()) : int(0);
            for (size_t i = 0; i < len; i++) {
                uint8_t v = d[i];
                f.write((const char *)&v, 1);
            }
        } else {
            assert(d.t >= i8 && d.t <= Bool);
            f.write((const char *)d.n, ts);
        }
    };
    
    if (format == var::Binary)
        wri(*this);
    else {
        auto json = str(*this);
        f.write(json.cstr(), json.length());
    }
}

var::operator std::string() {
    switch (t) {
        case i8:   [[fallthrough]];
        case ui8:  return std::to_string(*((int8_t  *)n));
        case i16:  [[fallthrough]];
        case ui16: return std::to_string(*((int16_t *)n));
        case i32:  [[fallthrough]];
        case ui32: return std::to_string(*((int32_t *)n));
        case i64:  [[fallthrough]];
        case ui64: return std::to_string(*((int64_t *)n));
        case f32:  return std::to_string(*((float   *)n));
        case f64:  return std::to_string(*((double  *)n));
        case Str:  return s ? *s : "";
        case Array: {
            std::string s = "[";
            if (c >= var::i8 && c <= var::Bool) {
                size_t sz = shape_volume(sh);
                size_t ts = type_size(c);
                uint8_t *v = d.get();
                for (size_t i = 0; i < sz; i++) {
                    if (i > 0)
                        s += ",";
                    if      (c == var::f32)                    s += std::to_string(*(float *)  &v[ts * i]);
                    else if (c == var::f64)                    s += std::to_string(*(double *) &v[ts * i]);
                    else if (c == var::i8  || c == var::ui8)   s += std::to_string(*(int8_t *) &v[ts * i]);
                    else if (c == var::i16 || c == var::ui16)  s += std::to_string(*(int16_t *)&v[ts * i]);
                    else if (c == var::i32 || c == var::ui32)  s += std::to_string(*(int32_t *)&v[ts * i]);
                    else if (c == var::i64 || c == var::ui64)  s += std::to_string(*(int64_t *)&v[ts * i]);
                    else if (c == var::Bool)                   s += std::to_string(*(bool *)   &v[ts * i]);
                    else
                        assert(false);
                }
            } else {
                assert(a); // this is fine
                for (auto &d: *a) {
                    if (s.length() != 1)
                        s += ",";
                    s += std::string(d);
                }
            }
            s += "]";
            return s;
        }
        case Map: {
            std::string s = "{";
            for (auto &[k,d]: *m) {
                if (s.length() != 1)
                    s += ",";
                s += std::string(k);
                s += ":";
                s += std::string(d);
            }
            s += "}";
            return s;
        }
        case Ref: {
            char buf[64];
            sprintf(buf, "%p", (void *)n);
            return std::string(buf);
        }
        default:
            break;
    }
    return "";
}

var::operator bool() {
    switch (t) {
        case i8:    return *((  int8_t *)n) > 0;
        case ui8:   return *(( uint8_t *)n) > 0;
        case i16:   return *(( int16_t *)n) > 0;
        case ui16:  return *((uint16_t *)n) > 0;
        case i32:   return *(( int32_t *)n) > 0;
        case ui32:  return *((uint32_t *)n) > 0;
        case i64:   return *(( int64_t *)n) > 0;
        case ui64:  return *((uint64_t *)n) > 0;
        case f32:   return *((float    *)n) > 0;
        case f64:   return *((double   *)n) > 0;
        case Str:   return      s->length() > 0;
        case Map:   return        m->size() > 0;
        case Array: return        a->size() > 0;
        default:
            break;
    }
    return false;
}

bool var::operator!() {
    return !(var::operator bool());
}

var::var(var::Type t, var::Type c, size_t sz)  : t(t == var::Undefined ? var::Map : t), c(
        ((t == var::Array || t == var::Map) && c == var::Undefined) ? var::Any : c) {
    if (this->t == Array) {
        a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
        a->reserve(sz);
    } else if (this->t == Map)
        m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
    else
        n = &n_value;
}

var::var(float    v)         : t(f32)  {
    n = &n_value;
    *((float *)n) = v;
}

var::var(double   v)         : t(f64)  {
    n = &n_value;
    *((double *)n) = v;
}

var::var(int8_t   v)         : t(i8)   {
    n = &n_value;
    *((int8_t *)n) = v;
}

var::var(uint8_t  v)         : t(ui8)  {
    n = &n_value;
    *((uint8_t *)n) = v;
}

var::var(int16_t  v)         : t(i16)  {
    n = &n_value;
    *((int16_t *)n) = v;
}

var::var(uint16_t v)         : t(ui16) {
    n = &n_value;
    *((uint16_t *)n) = v;
}

var::var(int32_t  v)         : t(i32)  {
    n = &n_value;
    *((int32_t *)n) = v;
}

var::var(uint32_t v)         : t(ui32) {
    n = &n_value;
    *((uint32_t *)n) = v;
}

var::var(int64_t  v)         : t(i64)  {
    n = &n_value;
    *((int64_t *)n) = v;
}

/*var::var(uint64_t v)       : t(ui64) {
    n = &n_value;
    *((uint64_t *)n) = v;
}*/

var::var(size_t v)           : t(ui64) {
    n = &n_value;
    *((uint64_t *)n) = uint64_t(v);
}

var::var(void* v)            : t(Ref) { n = (u *)v; }
var::var(VoidRef vr)         : t(Ref) { n = (u *)vr.v; }
var::var(var *ref)          : t(Ref) { n = (u *)ref; }
var::var(std::string str)    : t(Str), s(new std::string(str)) { }
var::var(const char *str)    : t(Str), s(new std::string(str)) { }
var::var(path_t p) : t(Type::Str), s(new std::string(p.string())) { }
var::var(const var &ref) {
    copy((var &)ref);
}

var var::read_json(str &js, ::map<std::string, TypeFlags> flags) {
    std::function<char *(var &, char *, std::string)> parse_obj, parse_arr, parse_value;
    
    parse_obj = [&](var &scope, char *start, std::string field) {
        char *cur = start;
        assert(*cur == '{');
        ws(&(++cur));
        scope.t = Map;
        scope.c = Any;
        scope.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
        while (*cur != '}') {
            auto field = parse_quoted(&cur);
            ws(&cur);
            assert(field != "");
            assert(*cur == ':');
            ws(&(++cur));
            scope[field] = var(Undefined);
            cur = parse_value(scope[field], cur, field);
            if (ws(&cur) == ',')
                ws(&(++cur));
        }
        assert(*cur == '}');
        ws(&(++cur));
        return cur;
    };
    
    parse_arr = [&](var &scope, char *start, std::string field) {
        char *cur = start;
        assert(*cur == '[');
        scope.t = Array;
        scope.c = Any;
        scope.a = std::shared_ptr<std::vector<var>>(new std::vector<var>());
        ws(&(++cur));
        for (;;) {
            scope.a->push_back(var(Undefined));
            cur = parse_value(scope.a->back(), cur, field);
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
    };
    
    parse_value = [&](var &scope, char *start, std::string field) {
        char *cur = start;
        if (*cur == '{') {
            return parse_obj(scope, start, field);
        } else if (*cur == '[') {
            return parse_arr(scope, start, field);
        } else if (*cur == 't' || *cur == 'f') {
            scope.n = &scope.n_value;
            scope.t = Bool;
            *((bool *)scope.n) = *cur == 't';
        } else if (*cur == '"') {
            std::string s = parse_quoted(&cur);
            if (flags.count(field) > 0 && (flags[field].flags & Flags::Encode)) {
                size_t alloc = 0;
                scope.t  = Array;
                scope.c  = flags[field].t;
                scope.d  = decode(s.c_str(), s.length(), &alloc);
                scope.sh = { int((alloc - 1) / type_size(scope.c)) };
                assert(alloc > 0);
            } else {
                scope.t = Str;
                scope.s = std::shared_ptr<std::string>(new std::string(s));
            }
        } else if (*cur == '-' || isdigit(*cur)) {
            std::string value = parse_numeric(&cur);
            assert(value != "");
            double d = atof(value.c_str());
            if (flags.count(field) > 0)
                scope.t = flags[field].t;
            else if (std::floor(d) != d)
                scope.t = f64;
            else
                scope.t = i32;
            scope.n = &scope.n_value;
            switch (scope.t) {
                case Bool: *((    bool *)scope.n) =     bool(d); break;
                case  ui8: *(( uint8_t *)scope.n) =  uint8_t(d); break;
                case   i8: *((  int8_t *)scope.n) =   int8_t(d); break;
                case ui16: *((uint16_t *)scope.n) = uint16_t(d); break;
                case  i16: *(( int16_t *)scope.n) =  int16_t(d); break;
                case ui32: *((uint32_t *)scope.n) = uint32_t(d); break;
                case  i32: *(( int32_t *)scope.n) =  int32_t(d); break;
                case ui64: *((uint64_t *)scope.n) = uint64_t(d); break;
                case  i64: *(( int64_t *)scope.n) =  int64_t(d); break;
                case  f32: *((   float *)scope.n) =    float(d); break;
                case  f64: *((  double *)scope.n) =   double(d); break;
                default:
                    assert(false);
                    break;
            }
        } else
            assert(false);
        return cur;
    };
    var r;
    parse_value(r, (char *)js.cstr(), "");
    return r;
}

var::var(path_t json, ::map<std::string, TypeFlags> flags) : var(Undefined) {
    if (!std::filesystem::is_regular_file(json))
        return;
    str js = str(json);
    read_json(js, flags);
}

var &var::operator[](str s) {
    return (*m)[s];
}

void var::compact() {
    if (!a || t != Array || (flags & Flags::Compact))
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
        size_t ts = type_size(Type(type));
        if (ts == 0)
            return;
        size_t sz = a->size();
        var   *r = a->data();
        assert(type >= i8 && type <= Bool);
        sh = { int(sz) };
        d  = std::shared_ptr<uint8_t>((uint8_t *)malloc(ts * sz));
        
        switch (t) {
            case var::Bool: {
                bool *v = (bool *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((bool *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::i8:
            case var::ui8: {
                int8_t *v = (int8_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int8_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::i16:
            case var::ui16: {
                int16_t *v = (int16_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int16_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::i32:
            case var::ui32: {
                int32_t *v = (int32_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int32_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::i64:
            case var::ui64: {
                int64_t *v = (int64_t *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((int64_t *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::f32: {
                float *v = (float *)d.get();
                for (size_t i = 0; i < sz; i++) {
                    v[i] = *((float *)r[i].n);
                    r[i].n = (u *)&v[i];
                }
                break;
            }
            case var::f64: {
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

var::~var() { }
var:: var(str s) : t(var::Str), s(new std::string(s.s)) { }

std::vector<int> var::shape() {
    if (sh.size())
        return sh;
    return std::vector<int> { size() ? int(size()) : int(type_size(t)) };
}

void var::set_shape(std::vector<int> s) {
    sh = s;
}

var var::tag(map<str, var> m) {
    var d = *this;
    assert(d.t != var::Map);
    d.m = std::shared_ptr<::map<std::string, var>>(new ::map<std::string, var>());
    for (auto &[k,v]: m)
        d[k] = v;
    return d;
}

size_t var::size() const {
    int sz = 1;
    for (auto d: sh)
        sz *= d;
    if (t == Array)
        return d ? sz : (a ? a->size() : 0);
    else if (t == Map)
        return m->size();
    else if (t == Str)
        return s ? s->length() : 0;
    return t == Undefined ? 0 : 1;
}

void var::each(FnEach each) {
    if (m)
        for(auto &[k,v]: *m) {
            var &d = v;
            std::string s = k; // useful to set const on the Key of the pair structure.
            each(d, s);
        }
}

void var::each(FnArrayEach each) {
    // never do a null check here, never.  do not cater to missing data
    size_t i = 0;
    if (a)
        for(auto &d: *a)
            each(d, i++);
}

void var::copy(var &ref) {
    t      = ref.t;
    c      = ref.c;
    d      = ref.d;
    a      = ref.a;
    s      = ref.s;
    m      = ref.m;
    sh     = ref.sh;
    flags  = ref.flags;

    if (ref.ff)
        ff = ref.ff;
    if (ref.fn)
        fn = ref.fn;
    
    if (ref.n == &ref.n_value) {
        n_value = ref.n_value;
        n = &n_value;
    } else if (ref.t == var::Ref)
        n = ref.n;
    else
        n = ref.n;
}

void var::operator += (var ref) {
    if (a)
      (*a).push_back(ref);
}

var& var::operator=(const var &ref) {
    if (this != &ref)
        copy((var &)ref);
    return *this;
}

bool var::operator!=(const var &ref) {
    if (t != ref.t)
        return true;
    else if (t == var::Undefined || t == var::Lambda || t == var::Filter || t == var::Any)
        return false;
    else if (t == var::Str)
        return *s != *(ref.s);
    else if (t == var::Map) {
        if (m ? m->size() : 0 != ref.m ? ref.m->size() : 0)
            return true;
        if (m)
            for (auto &[k,d]: *m) {
                if (ref.m->count(k) == 0)
                    return true;
                auto &rd = (*(ref.m))[k];
                if (d != rd)
                    return true;
            }
        return true;
    } else if (t == var::Array) {
        if (a->size() != ref.a->size())
            return true;
        size_t k = 0;
        for (auto &d: *a) {
            auto &rd = (*(ref.a))[k++];
            if (d != rd)
                return true;
        }
    } else if (t == var::Ref) {
        assert(false); // todo
    } else {
        assert(t >= var::i8 && t <= var::Bool);
        size_t  ts = type_size(t);
        bool match = memcmp(n, ref.n, ts) == 0;
        return !match;
    }
    return true;
}

bool var::operator==(::map<std::string, var> &map) const {
    if (!m)
        return map.size() == 0;
    return map == *m;
}

bool var::operator!=(::map<std::string, var> &map) const {
    return !operator==(map);
}

bool var::operator==(const var &ref) {
    return !(operator!=(ref));
}

bool var::operator==(var &ref) {
    return !(operator!=((const var &)ref));
}

size_t var::type_size(var::Type t) {
    switch (t) {
        case var::Bool:  return 1;
        case var::i8:    return 1;
        case var::ui8:   return 1;
        case var::i16:   return 2;
        case var::ui16:  return 2;
        case var::i32:   return 4;
        case var::ui32:  return 4;
        case var::i64:   return 8;
        case var::ui64:  return 8;
        case var::f32:   return 4;
        case var::f64:   return 8;
        default:
            break;
    }
    return 0;
}
