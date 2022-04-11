/// convert to seed-based var design, just an array or queue of data structures, shared structures.
///

import <iostream>
import <unordered_map>
import <map>
import <stack>
import <array>
import <vector>
import <filesystem>
import <iostream>
import <fstream>
import <sstream>
import <chrono>
import <limits>
import <cmath>
import <functional>
import <cstring>
import <random>
import <type_traits>

#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <unistd.h>

import dx:array;
import dx:mappy;
import dx:string;

export module dx:var;
export {

Logger<LogType::Dissolvable> console;

void _print(str t, ::array<var> &a, bool error) {
    t = var::format(t, a);
    auto &out = error ? std::cout : std::cerr;
    out << str(t) << std::endl << std::flush;
}

static inline
int64_t ticks() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

template <class T>
static inline T interp(T from, T to, T f) { return from * (T(1) - f) + to * f; }

template <typename T>
static inline T *allocate(size_t count, size_t align = 16) {
    size_t sz = count * sizeof(T); return (T *)aligned_alloc(align, (sz + (align - 1)) & ~(align - 1));
}

template <typename T>
static inline void memclear(T *dst, size_t count = 1, int value = 0) { memset(dst, value, count * sizeof(T)); }

template <class T>
static inline void memclear(T &dst, size_t count = 1, int value = 0) { memset(&dst, value, count * sizeof(T)); }

template <typename T>
static inline void memcopy(T *dst, T *src, size_t count = 1)         { memcpy(dst, src, count * sizeof(T)); }

struct VoidRef {
    void *v;
    VoidRef(void *v) : v(v) { }
};

struct var:io {
    Type::Specifier t = Type::Undefined;
    Type::Specifier c = Type::Undefined;

    enum Binding {
        Insert,
        Update,
        Delete
    };

    enum Flags {
        Encode   = 1,
        Compact  = 2,
        ReadOnly = 4,
        DestructAttached = 8
    };
    
    struct TypeFlags {
        Type::Specifier t;
        int flags;
    };
    
    typedef map<string, TypeFlags> FieldFlags;
    
    enum Format {
        Binary,
        Json
    };
    
    union u {
        bool    vb;
        int8_t  vi8;
        int16_t vi16;
        int32_t vi32;
        int64_t vi64;
        float   vf32;
        double  vf64;
        var    *vref;
        node   *vnode;
        void   *vstar;
    };
    
    u                            n_value = { 0 }; /// should be n space (n points to unions elsewhere too such as in-stride shorts, very out of fashion)
    Fn                           fn;
  //FnFilter                     ff;
    std::shared_ptr<uint8_t>     d = null;
    map<string, var>             m = null;
    ::array<var>                 a = null;
    str                          s = null;
    u                           *n = null;
    ///
    int                          flags = 0; // attempt to deprecate
    Shape<Major>                 sh;
//protected:
    /// observer fn and pointer (used by Storage controllers -- todo: merge into binding allocation via class
    std::function<void(Binding op, var &row, str &field, var &value)> fn_change;
    string     field = "";
    var         *row = null;
    var    *observer = null;
    ///
    void observe_row(var &row);
    static var &resolve(var &in);
    static char *parse_obj(  var &scope, char *start, string field, FieldFlags &flags);
    static char *parse_arr(  var &scope, char *start, string field, FieldFlags &flags);
    static char *parse_value(var &scope, char *start, string field, Type::Specifier enforce_type, FieldFlags &flags);
    ///
public:



    //#include <dx/io.hpp>
    //#include <dx/map.hpp>
    //#include <dx/type.hpp>



    bool   is_numeric   (cchar_t *s)    { return (s && (*s == '-' || isdigit(*s))); }

    str parse_numeric(char **cursor) {
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
        str result = "";
        for (int i = 0; i < int(number_len); i++)
            result += number_start[i];
        return result;
    }

    /// \\ = \ ... \x = \x
    str parse_quoted(char **cursor, size_t max_len = 0) {
        cchar_t *first = *cursor;
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
        str result = "";
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

    bool type_check(var &a, var &b) {
        var &aa = resolve(a);
        var &bb = resolve(b);
        return aa.t == bb.t && aa.c == bb.c;
    }

    var(std::nullptr_t p) : var(Type::Undefined) { }
    var(::array<var> da)  : t(Type::Array)       {
        auto sz = da.size();
        c       = sz ? da[0].t : Type::Undefined;
        a       = ::array<var>(sz);
        for (auto &d: da)
            a += d;
    }

    bool operator==(uint16_t v) { return n && *((uint16_t *)n) == v; }
    bool operator==( int16_t v) { return n && *(( int16_t *)n) == v; }
    bool operator==(uint32_t v) { return n && *((uint32_t *)n) == v; }
    bool operator==( int32_t v) { return n && *(( int32_t *)n) == v; }
    bool operator==(uint64_t v) { return n && *((uint64_t *)n) == v; }
    bool operator==( int64_t v) { return n && *(( int64_t *)n) == v; }
    bool operator==( float   v) { return n && *((   float *)n) == v; }
    bool operator==(double   v) { return n && *((  double *)n) == v; }

    var(Fn fn_) : t(Type::Lambda) {
        fn = fn_;
    }

    bool operator==(Type::Specifier tt) {
        return resolve(*this).t == tt;
    }

    var select_first(std::function<var (var &)> fn) {
        var &r = resolve(*this);
        if (r.a)
            for (auto &v: r.a) {
                auto ret = fn(v);
                if (ret)
                    return ret;
            }
        return null;
    }

    ::array<var> select(std::function<var(var &)> fn) {
        ::array<var> result;
        var &r = resolve(*this);
        if (r.a)
            for (auto &v: r.a) {
                var add = fn(v);
                if (add)
                    result += add;
            }
        return result;
    }

    void clear() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return;
        v.n_value = {};
        if (v.t == Type::Array)
            v.a = ::array<var>();
        if (v.t == Type::Map)
            v.m = ::map<str, var>();
    }

    ::map<int, int> b64_encoder() {
        auto m = map<int, int>();
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

    std::shared_ptr<uint8_t> encode(cchar_t *data, size_t len) {
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

    ::map<int, int> b64_decoder() {
        auto m = ::map<int, int>();
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

    std::shared_ptr<uint8_t> decode(cchar_t *b64, size_t b64_len, size_t *alloc) {
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

    /// replace in place with var next. its a type, and a shared allocation, windowed access with just reduced types and members at play
    /// try to go flags-less (not used very much now, we can depart there)
    void reserve(size_t sz) { if (sz) a.reserve(sz); }
    operator path_t()       { return path_t(s); }

    var(path_t p, enum Format format) {
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

    void write(path_t p, enum Format format) {
        std::ofstream f(p, std::ios::out | std::ios::binary);
        write(f, Binary);
        f.close();
    }

    // construct data from file ref
    var(std::ifstream &f, enum Format format) {
        auto st = [&]() -> str {
            int len;
            f.read((char *)&len, sizeof(int));
            char *v = (char *)malloc(len + 1);
            f.read(v, len);
            v[len] = 0;
            return str(v, len);
        };
        
        auto rd = std::function<void(var &)>();
        rd = [&](var &d) {
            int dt;
            f.read((char *)&dt, sizeof(int));
            d.t = Type::Specifier(size_t(dt));
            
            int meta_sz = 0;
            f.read((char *)&meta_sz, sizeof(int));
            for (int i = 0; i < meta_sz; i++) {
                str key = st();
                rd(d[key]);
            }
            if (d == Type::Array) {
                int sz;
                f.read((char *)&d.c, sizeof(int));
                f.read((char *)&sz,  sizeof(int));
                if (sz) {
                    ::array<int> sh;
                    int s_sz;
                    f.read((char *)&s_sz, sizeof(int));
                    for (int i = 0; i < s_sz; i++) {
                        sh += 0;
                        f.read((char *)&sh[sh.size() - 1], sizeof(int));
                    }
                    d.sh = sh;
                    if (d.c >= Type::i8 && d.c <= Type::Bool) {
                        int ts = Type::size(d.c);
                        d.d = std::shared_ptr<uint8_t>(new uint8_t[ts * sz]);
                        f.read((char *)d.data<uint8_t>(), ts * sz);
                    } else {
                        d.a = ::array<var>(sz);
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
                    str key = st();
                    rd(d[key]);
                }
            } else if (d == Type::Str) {
                int len;
                f.read((char *)&len, sizeof(int));
                d.s = str(st());
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

    void write(std::ofstream &f, enum Format format) {
        auto wri_str = std::function<void(str &)>();
        
        wri_str = [&](str &str) {
            int len = int(str.size());
            f.write((char *)&len, sizeof(int));
            f.write(str.cstr(), size_t(len));
        };
        
        auto wri = std::function<void(var &)>();
        
        wri = [&](var &d) {
            int dt = d.t;
            f.write((cchar_t *)&dt, sizeof(int));
            Size ts = Type::size(d.t);
            
            // meta != map in abstract; we just dont support it at runtime
            Size meta_sz = ((d.t != Type::Map) && d.m) ? d.m.size() : 0;
            f.write((cchar_t *)&meta_sz, sizeof(int));
            if (meta_sz)
                for (auto &[k,v]: d.m) {
                    str kname = k;
                    wri_str(k);
                    wri(v);
                }
            
            if (d == Type::Array) {
                int     idc = d.c;
                int      sz = d.a ? int(d.a.size()) : int(d.sh);
                int    s_sz = int(d.sh.size());
                f.write((cchar_t *)&idc, sizeof(int));
                f.write((cchar_t *)&sz, sizeof(int));
                if (sz) {
                    if (d.a) {
                        int i1 = 1;
                        int sz = d.size();
                        f.write((cchar_t *)&i1, sizeof(int));
                        f.write((cchar_t *)&sz, sizeof(int));
                    } else {
                        f.write((cchar_t *)&s_sz, sizeof(int));
                        ::array<int> ishape = d.sh;
                        for (int i:ishape)
                            f.write((cchar_t *)&i, sizeof(int));
                    }
                    if (d.c >= Type::i8 && d.c <= Type::Bool) {
                        assert(d.d);
                        size_t ts = Type::size(d.c);
                        f.write((cchar_t *)d.data<uint8_t>(), ts * size_t(sz));
                    } else if (d.c == Type::Map) {
                        assert(d.a);
                        for (auto &i: d.a)
                            wri(i);
                    }
                }
            } else if (d == Type::Map) {
                assert(d.m);
                int sz = int(d.m.size());
                f.write((cchar_t *)&sz, sizeof(int));
                for (auto &[k,v]: d.m) {
                    wri_str(k);
                    wri(v);
                }
            } else if (d == Type::Str) {
                int len = d.s ? int(d.s.size()) : int(0);
                for (size_t i = 0; i < len; i++) {
                    uint8_t v = d[i];
                    f.write((cchar_t *)&v, 1);
                }
            } else {
                assert(d.t >= Type::i8 && d.t <= Type::Bool);
                f.write((cchar_t *)d.n, ts);
            }
        };
        
        var &v = resolve(*this);
        if (format == Binary)
            wri(v);
        else {
            auto json = str(v);
            f.write(json.cstr(), json.size());
        }
    }

    // silver-c. #join with your friend at first.
    operator str() {
        var &r = resolve(*this);
        switch (size_t(r.t)) {
            case Type::i8:     [[fallthrough]];
            case Type::ui8:  return std::to_string(*((int8_t  *)r.n));
            case Type::i16:    [[fallthrough]];
            case Type::ui16: return std::to_string(*((int16_t *)r.n));
            case Type::i32:    [[fallthrough]];
            case Type::ui32: return std::to_string(*((int32_t *)r.n));
            case Type::i64:    [[fallthrough]];
            case Type::ui64: return std::to_string(*((int64_t *)r.n));
            case Type::f32:  return std::to_string(*((float   *)r.n));
            case Type::f64:  return std::to_string(*((double  *)r.n));
            case Type::Str:  return r.s;
            case Type::Array: {
                str ss = "[";
                if (c >= Type::i8 && c <= Type::Bool) {
                    size_t  sz = size_t(sh);
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
                    for (auto &value: r.a) {
                        if (ss.size() != 1)
                            ss += ",";
                        ss += str(value);
                    }
                }
                ss += "]";
                return ss;
            }
            case Type::Map: {
                if (r.s)
                    return r.s; /// use-case: var-based Model names
                str ss = "{";
                if (r.m)
                    for (auto &[field, value]: r.m) {
                        if (ss.size() != 1)
                            ss += ",";
                        ss += str(field);
                        ss += ":";
                        if (value == Type::Str) {
                            ss += "\"";
                            ss += str(value);
                            ss += "\"";
                        } else
                            ss += str(value);
                    }
                ss += "}";
                return ss;
            }
            case Type::Ref: {
                if (r.n) {
                    char buf[64];
                    sprintf(buf, "%p", (void *)r.n);
                    return str(buf);
                }
                return str(""); // null reference; context needed for null str
            }
            default:
                break;
        }
        return "";
    }

    operator bool() {
        var &v = resolve(*this);
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
            case Type::Str:   return         v.s.size() > 0;
            case Type::Map:   return        v.m and v.m.size() > 0;
            case Type::Array: return        v.d  or v.a.size() > 0;
            case Type::Ref:   return v.n_value.vref ? bool(v.n_value.vref) : bool(v.n);
            default:
                break;
        }
        return false;
    }

    bool operator!() {
        return !(operator bool());
    }

    var(Type::Specifier c, std::shared_ptr<uint8_t> d, Shape<Major> sh) : t(Type::Array), c(c), d(d), sh(sh) { }

    var(Type::Specifier t, Type::Specifier c, size_t sz)  : t(t == Type::Undefined ? Type::Map : t), c(
            ((t == Type::Array || t == Type::Map) && c == Type::Undefined) ? Type::Any : c) {
        if (this->t == Type::Array)
            a = ::array<var>(sz);
        else if (this->t == Type::Map)
            m = ::map<str, var>();
        else
            n = &n_value;
    }

    var(float    v) : t(Type:: f32), n(&n_value) { *((  float  *)n) = v; }
    var(double   v) : t(Type:: f64), n(&n_value) { *(( double  *)n) = v; }
    var(int8_t   v) : t(Type::  i8), n(&n_value) { *(( int8_t  *)n) = v; }
    var(uint8_t  v) : t(Type:: ui8), n(&n_value) { *((uint8_t  *)n) = v; }
    var(int16_t  v) : t(Type:: i16), n(&n_value) { *((int16_t  *)n) = v; }
    var(uint16_t v) : t(Type::ui16), n(&n_value) { *((uint16_t *)n) = v; }
    var(int32_t  v) : t(Type:: i32), n(&n_value) { *((int32_t  *)n) = v; }
    var(uint32_t v) : t(Type::ui32), n(&n_value) { *((uint32_t *)n) = v; }

    /*var(uint64_t v) : t(Type::ui64) {
        n = &n_value;
        *((uint64_t *)n) = v;
    }*/

    var(size_t v) : t(Type::ui64) {
        n = &n_value;
        *((uint64_t *)n) = uint64_t(v);
    }

    /// merge these two use cases into VoidRef, Ref, singular Ref struct (likely quite usable for data Modeling as well)
    var(void* v)         : t(Type::Ref)                { n = (u *)v;    }
    var(VoidRef vr)      : t(Type::Ref)                { n = (u *)vr.v; }
    var(str       str)   : t(Type::Str), s(str)        { }
    var(cchar_t *cstr)   : t(Type::Str), s(cstr)       { }
    var(path_t p)        : t(Type::Str), s(p.string()) { }
    var(const var &ref) {
        copy((var &)ref);
    }

    char *parse_obj(var &scope, char *start, str field, FieldFlags &flags) {
        char *cur = start;
        assert(*cur == '{');
        ws(&(++cur));
        scope.t = Type::Map;
        scope.c = Type::Any;
        scope.m = ::map<string, var>();
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

    char *parse_arr(var &scope, char *start, string field, FieldFlags &flags) {
        char *cur = start;
        assert(*cur == '[');
        scope.t = Type::Array;
        scope.c = Type::Any;
        scope.a = ::array<var>();
        ws(&(++cur));
        if (*cur == ']')
            ws(&(++cur));
        else {
            for (;;) {
                scope.a += var(Type::Undefined);
                cur = parse_value(scope.a.back(), cur, field, Type::Any, flags);
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
        }
        if (flags.count(field) > 0)
            scope.flags = flags[field].flags;
        scope.compact();
        return cur;
    }

    char *parse_value(var &scope, char *start, string field,
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
            string s = parse_quoted(&cur);
            if (flags.count(field) > 0 && (flags[field].flags & Flags::Encode)) {
                assert(enforce_type == Type::Any || enforce_type == Type::Array);
                size_t alloc = 0;
                scope.t  = Type::Array;
                scope.c  = flags[field].t;
                scope.d  = decode(s.cstr(), s.size(), &alloc);
                scope.sh = (alloc - 1) / Type::size(scope.c);
                assert(alloc > 0);
            } else {
                assert(enforce_type == Type::Any || enforce_type == Type::Str);
                scope.t = Type::Str;
                scope.s = s;
            }
        } else if (*cur == '-' || isdigit(*cur)) {
            string value = parse_numeric(&cur);
            assert(value != "");
            double  d = atof(value.cstr());
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
                scope.s = value;
            }
        } else
            assert(false);
        return cur;
    }

    /// call this from json
    var(Type::Specifier t, string str) : t(t) {
        cchar_t *c = str.cstr();
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
                s = str;
                break;
            }
            default: {
                /// invalid general usage at the moment
                assert(false);
                break;
            }
        }
        
    }

    var parse_json(str &js) {
        var r;
        FieldFlags flags;
        parse_value(r, (char *)js.cstr(), "", Type::Any, flags);
        return r;
    }

    var load(path_t p) {
        if (!std::filesystem::is_regular_file(p))
            return null;
        str js = str::read_file(p);
        return parse_json(js);
    }

    var &operator[](str s) {
        str ss = s;
        var &v = resolve(*this);
        return v.m[s];
    }

    void compact() {
        if (!a || t != Type::Array || (flags & Flags::Compact))
            return;
        flags    |= Flags::Compact;
        int type  = -1;
        int types =  0;
        
        for (auto &i: a)
            if (i.t != type) {
                type = i.t;
                types++;
            }
        
        if (types == 1) {
            size_t ts = Type::size(Type::Specifier(size_t(type)));
            if (ts == 0)
                return;
            sh     = a.size();
            var *r = a.data();
            d      = std::shared_ptr<uint8_t>((uint8_t *)malloc(ts * sh));
            
            switch (type) {
                case Type::Bool: {
                    bool *v = (bool *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((bool *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::i8:
                case Type::ui8: {
                    int8_t *v = (int8_t *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((int8_t *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::i16:
                case Type::ui16: {
                    int16_t *v = (int16_t *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((int16_t *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::i32:
                case Type::ui32: {
                    int32_t *v = (int32_t *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((int32_t *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::i64:
                case Type::ui64: {
                    int64_t *v = (int64_t *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((int64_t *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::f32: {
                    float *v = (float *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((float *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                case Type::f64: {
                    double *v = (double *)d.get();
                    for (size_t i = 0; i < sh; i++) {
                        v[i] = *((double *)r[i].n);
                        r[i].n = (u *)&v[i];
                    }
                    break;
                }
                default:
                    break;
            }
        } else
            assert(types == 0);
    }

    // resolve from vrefs potentially
    var &resolve(var &in) {
        var    *p = &in;
        while  (p && p->t == Type::Ref)
                p =  p->n_value.vref;
        return *p;
    }

    void notify_update() {
        var &v = resolve(*this);
        if (!v.observer)
            return;
        if (v.row) {
            /// [design] row is not set on new rows, because it has not yet been added.
            assert(v.t != Type::Ref);
            assert(v.field != "");
            str   s_field = v.field;
            observer->fn_change(Binding::Update, *v.row, s_field, v); // row needs to be made vref through indirection if needed
        }
    }

    void notify_insert() {
        var &v = resolve(*this);
        if (!v.observer)
            return;
        assert( v.t != Type::Ref);
        assert(!v.row);
        str snull = null;
        var vnull = null; /// print observer values; they should all point to the same var
        observer->fn_change(Binding::Insert, v, snull, vnull);
    }

    void notify_delete() {
        var &v = resolve(*this);
        if (!v.observer)
            return;
        assert( v.t != Type::Ref);
        assert(!v.row);
        str snull = null;
        var vnull = null;
        observer->fn_change(Binding::Delete, v, snull, vnull);
    }

    ~var() {
        if (flags & DestructAttached)
            fn(*this);
    }

    /// in a few days we got Shape, Size, new Map, new String, new Var, deleted Var, fixed Var
    Shape<Major> shape() {
        var &v = resolve(*this);
        Shape<Major> sm;
        if (v.sh.size())
            sm = v.sh;
        else if (v.size())
            sm = v.size();
        else
            sm = Type::size(v.t);
        return sm;
    }

    void set_shape(Shape<Major> &s) {
        var &v = resolve(*this);
        v.sh = s;
    }

    var tag(::map<string, var> m) {
        var v = copy();
        assert(v.t != Type::Map);
        for (auto &[field, value]:v.m)
            v[field] = value;
        return v;
    }

    Size size() {
        var &v = resolve(*this);
        int sz = 1;
        for (auto d: v.sh)
            sz *= d;
        if      (v.t == Type::Array) return v.d ? Size(sz) : v.a.size();
        else if (v.t == Type::Map)   return   m.size();
        else if (v.t == Type::Str)   return v.s.size();
        return   v.t == Type::Undefined ? 0 : 1;
    }

    void each(FnEach each) {
        var &v = resolve(*this);
        for(auto &[field, value]: v.m)
            each(value, field);
    }

    void each(FnArrayEach each) { size_t i = 0; for (auto &d: a) each(d, i++); }

    /// once we get an assign= operator, you are notified with the value, and you call update
    void observe_row(var &row) {
        var &o = resolve(*this);
        var &r = resolve(row);
        r.observer = o.observer ? o.observer : o.ptr();
        for (auto &[field, value]: r.map()) {
            var &v     = resolve(value);
            ///
            v.observer = r.observer;
            v.field    = field;
            v.row      = r.ptr();
        }
    }

    /// once we get an assign= operator, you are notified with the value, and you call update
    void observe(std::function<void(Binding op, var &row, str &field, var &value)> fn) {
        var &v = resolve(*this);
        assert(v.t == Type::Array);
        for (auto &row: v.a)
            v.observe_row(row);
        v.observer  = v.ptr(); /// bug fix: i think that was a boolean op again.
        v.fn_change = fn;
    }

    /// external copy; refs as resolved in this process
    var copy() {
        var &v = resolve(*this);
        if (v.t == Type::Map) {
            var res = { v.t, v.c }; // res is a new null value of type:compound-type
            for (auto &[field, value]: v.map())
                res[field] = value;
            return res;
        } else if (v.t == Type::Array) {
            var res = { v.t, v.c };
            if (a) for (auto &e: v.a)
                res += e;
            return res;
        } else
            return *this;
    }

    /// interal copy; refs copy as refs; this is part of a normal copy process
    void copy(var &ref) {
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

    void operator+= (var ref) {
        var  &v = resolve(*this);
        v.a += ref;
        if (v.observer) {
            var &e = v.a[v.a.size() - 1];
            v.observe_row(e);  /// update bindings
            e.notify_insert(); ///
        }
    }

    var& operator = (const var &ref) {
        if (this != &ref) {
            copy((var &)ref);
            /// post-copy
            // dx revisit after demo apps
            //if (!(flags & ReadOnly))
            //    notify_update();
        }
        return *this;
    }

    bool operator!= (const var &ref) {
        var  &v = resolve(*this);
        
        if (v.t != ref.t)
            return true;
        else if (v == Type::Lambda)
            return fn_id((Fn &)ref.fn) != fn_id((Fn &)fn);
        else if (v.t == Type::Undefined || v.t == Type::Any)
            return false;
        else if (v.t == Type::Str)
            return v.s != ref.s;
        else if (v.t == Type::Map) {
            if (v.m.size() != ref.m.size())
                return true;
            for (auto &[field, value]: v.m) {
                if (ref.m.count(field) == 0)
                    return true;
                auto &rd   = ((::map<string, var> &)ref.m)[field];
                if (value != rd)
                    return true;
            }
            return true;
        } else if (v.t == Type::Array) {
            size_t a_sz =   v.a.size();
            size_t b_sz = ref.a.size();
            if (a_sz != b_sz)
                return true;
            size_t i = 0;
            if (v.a)
                for (auto &value: v.a) {
                    auto &rd = ((::array<var> &)ref.a)[i++];
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

    bool operator==(::map<string, var> &m)     { return resolve(*this).m == m; }
    bool operator!=(::map<string, var> &map)   { return !operator==(map); }
    bool operator==(const var &ref)          { return !(operator!=(ref)); }
    bool operator==(var &ref)                { return !(operator!=((const var &)ref)); }

    void write_binary(path_t p) {
        assert(a);
        assert(t == Type::Array);
        uint8_t *vdata = data<uint8_t>();
        ///
        std::ofstream f(p, std::ios::out | std::ios::binary);
        f.write((cchar_t *)vdata, a.size() * Type::size(c));
        f.close();
    }

    std::unordered_map<Type, Symbols> symbols;

    /// parse arguments
    Map::Map(int argc, cchar_t* argv[], Map def, string first_field) {
        Map     &m = *this;
        string key = "";
        for (int i = 1; i < argc; i++) {
            auto a = argv[i];
            if (a[0] == '-' && a[1] == '-')
                key  = string(&a[2], strlen(&a[2]));
            if (i == 1 && first_field != "")
                m[first_field] = string(a);
            else if (key  != "") {
                str val = argv[i + 1];
                m[key]  = val.numeric() ? var(val.real()) : var(val);
                key     = "";
            }
        }
    }

    /// attach to enum? so map with name, those names app-based or general
    Symbols Build::Role::symbols = {
        { Undefined,        "undefined" },
        { Terminal,         "terminal"  },
        { Mobile,           "mobile"    },
        { Desktop,          "desktop"   },
        { AR,               "ar"        },
        { VR,               "vr"        },
        { Cloud,            "cloud"     }};

    Symbols Build::OS::symbols = {
        { Undefined,        "undefined" },  /// Arb behaviour with this null state
        { macOS,            "macOS"     },  /// Simple binary
        { Linux,            "Linux"     },  /// Simple binary
        { Windows,          "Windows"   },  /// Universal App Packaging (if its actually a thing for compiled apps)
        { iOS,              "iOS"       },  /// Universal App Packaging
        { Android,          "Android"   }}; /// Universal Binaries as done with the bin images in zip

    Symbols Build::Arch::symbols = {
        { Undefined,        "undefined" },
        { x86,              "x86"       },
        { x86_64,           "x86_64"    },
        { Arm32,            "arm32"     },
        { Arm64,            "arm64"     }, 
        { Mx,               "Mx"        }};

    ///
    var *ptr() { return this; }
    void clear();
    bool operator==(Type::Specifier tt);
    void observe(std::function<void(Binding op, var &row, str &field, var &value)> fn);
    void write(std::ofstream &f, enum Format format = Binary);
    void write(path_t p, enum Format format = Binary);
    void write_binary(path_t p);
    ///
    var(std::nullptr_t p);
    var(Type::Specifier t, void *v) : t(t), sh(0) {
        assert(t == Type::Arb);
        /// unsafe pointer use-case. [merge]
        n_value.vstar = (void *)v;
    }
    var(Type::Specifier t = Type::Undefined, Type::Specifier c = Type::Undefined, size_t count = 0);
    var(Type::Specifier t, u *n) : t(t), n((u *)n) {
        /// unsafe pointer use-case. [merge]
        // assert(false);
    }
    var(Type::Specifier t, string str);
    var(Size   sz) : t(Type::i64) { n_value.vi64 = ssize_t(sz); n = &n_value; } // clean this area up. lol [gets mop; todo]
    var(var *vref) : t(Type::Ref), n(null) {
        assert(vref);
        while (vref->t == Type::Ref)
            vref = vref->n_value.vref; // refactor the names.
        n_value.vref = vref;
    }
    var(path_t p, Format format);
    
    Shape<Major> shape();
    void set_shape(Shape<Major> &v_shape);
    void attach(string name, void *arb, std::function<void(var &)> fn) {
        auto &m = map();
        m[name]        = var { Type::Arb, arb };
        m[name].flags |= DestructAttached;
        m[name].fn     = fn;
    }

    size_t attachments() {
        size_t r = 0;
        for (auto &[k,v]:map())
            if (v.flags & DestructAttached)
                r++;
        return r;
    }

    var tag(map<string, var> m);
    var(std::ifstream &f, enum Format format);
    
    var(Type::Specifier c, Shape<Major> sh) : sh(sh) {
        size_t t_sz = Type::size(c);
        this->t = Type::Array;
        this->c = c;
        this->d = std::shared_ptr<uint8_t>((uint8_t *)calloc(t_sz, size_t(sh)));
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, Shape<Major> sh) : sh(sh) {
        this->t = Type::Array;
        this->c = c;
        this->d = std::static_pointer_cast<uint8_t>(d);
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    template <typename T>
    var(Type::Specifier c, std::shared_ptr<T> d, Size sz) {
        this->t  = Type::Array;
        this->c  = c;
        this->d  = std::static_pointer_cast<uint8_t>(d);
        this->sh = Shape<Major>(sz); /// Major.
        assert(c >= Type::i8 && c <= Type::f64);
    }
    
    var(Type::Specifier c, std::shared_ptr<uint8_t> d, Shape<Major> sh);
    var(path_t p);
    operator path_t();
    static bool type_check(var &a, var &b);
    var(void*    v);
    var(VoidRef  r);
    var(size_t  sz);
    var(float    v);
    var(double   v);
    var(int8_t   v);
    var(uint8_t  v);
    var(int16_t  v);
    var(uint16_t v);
    var(int32_t  v);
    var(uint32_t v);
    var(int64_t  v, int flags = 0);
    var(Fn       v);
    var(::array<var> data_array);
    var(string str);
    var(const char *str);
    
    static var load(path_t p);
    
    template <typename T>
    operator ::array<T>() {
        var &v = resolve(*this);
        assert(t == Type::Array);
        ::array<T> result(v.size());
        for (auto &i:v.a)
            result += T(i);
        return result;
    }
    
    map<string, var> &map()   { return resolve(*this).m; }
    ::array<var>     &array() { return resolve(*this).a; }
    var   operator()(var &d) {
        var &v = resolve(*this);
        var res;
        if (v == Type::Lambda)
            fn(d);
        return res;
    }
    
    template <typename T>
    T convert() {
        //var &v = resolve(*this);
        //assert(v == Type::ui8);
        assert(d != null);
        assert(size() == sizeof(T)); /// C3PO: that isn't very reassuring.
        return T(*(T *)d.get());
    }
    
    template <typename T>
    T value(string key, T default_v) {
        var &v = resolve(*this);
        return (v.m.count(key) == 0) ? default_v : T(v.m[key]);
    }
    
    template <typename K, typename T>
    var(::map<K, T> mm) : t(Type::Map) {
        for (auto &[k, v]: mm)
            m[k] = var(v);
    }
    
    template <typename T>
    var(::array<T> aa) : t(Type::Array) {
        T     *va = aa.data();
        Size   sz = aa.size();
        c         = Type::spec(va);
        a.reserve(sz);
        ///
        if constexpr ((std::is_same_v<T,     bool>)
                   || (std::is_same_v<T,   int8_t>) || (std::is_same_v<T,  uint8_t>)
                   || (std::is_same_v<T,  int16_t>) || (std::is_same_v<T, uint16_t>)
                   || (std::is_same_v<T,  int32_t>) || (std::is_same_v<T, uint32_t>)
                   || (std::is_same_v<T,  int64_t>) || (std::is_same_v<T, uint64_t>)
                   || (std::is_same_v<T,    float>) || (std::is_same_v<T,   double>)) {
            size_t ts = Type::size(c);
            sh        = Shape<Major>(sz);
            d         = std::shared_ptr<uint8_t>((uint8_t *)calloc(ts, sz));
            flags     = Flags::Compact;
            T *vdata  = (T *)d.get();
            memcopy(vdata, va, sz);
            assert(Type::spec(vdata) == c);
            for (size_t i = 0; i < sz; i++)
                a += var {c, (u *)&vdata[i]};
          } else
              for (size_t i = 0; i < sz; i++)
                  a += aa[i];
    }
    
    template <typename T>
    var(T *v, ::array<int> sh) : t(Type::Array), c(Type::spec(v)), sh(sh) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[size_t(sh)]);
        memcopy(d.get(), (uint8_t *)v, size_t(sh) * sizeof(T));
        flags = Compact;
    }
    
    template <typename T>
    var(T *v, Size sz) : t(Type::Array), c(Type::spec(v)), sh(sz) {
        d = std::shared_ptr<uint8_t>((uint8_t *)new T[sz]);
        memcopy(d.get(), (uint8_t *)v, size_t(sh) * sizeof(T));
        flags = Compact;
    }

         operator ::map<string, var> &() { return m; }
    var &operator[] (const char *s)      { return resolve(*this).m[string(s)]; }
    var &operator[] (str s);
    var &operator[] (const size_t i)    {
        var &v = resolve(*this);
        int sz = int(v.sh);
        bool needs_refs = (sz > 0 && !v.a);
        if (needs_refs) {
            uint8_t *u8 = (uint8_t *)v.d.get();
            size_t   ts = Type::size(v.c);
            v.a.clear(sz); 
            for (int i = 0; i < sz; i++)
                v.a += var { v.c, (u *)&u8[i * ts] };
        }
        return v.a[i];
    }
    
    size_t count(string fv) {
        var &v = resolve(*this);
        if (v.t == Type::Array) {
            if (a) {
                var  conv = fv;
                int count = 0;
                for (auto &v: a)
                    if (conv == v)
                        count++;
                return count;
            }
        }
        return v.m.count(fv);
    }
    
    template <typename T>
    operator std::shared_ptr<T> () {
        return std::static_pointer_cast<T>(d);
    }
    
    operator         Fn&() { return fn; }

    operator      int8_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return int8_t(*v.n_value.vref);
        return v.n ? *(( int8_t *)v.n) : 0;
    }

    operator     uint8_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return uint8_t(*v.n_value.vref);
        return v.n ? *((uint8_t *)v.n) : 0;
    }

    operator     int16_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return v.n ? *(( int16_t *)v.n) : 0;
    }

    operator    uint16_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return uint16_t(*v.n_value.vref);
        return v.n ? *((uint16_t *)v.n) : 0;
    }

    operator     int32_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return int32_t(*v.n_value.vref);
        if (v.t == Type::Str)
            return v.s.integer();
        return v.n ? *(( int32_t *)v.n) : 0;
    }

    operator    uint32_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return uint32_t(*v.n_value.vref);
        return v.n ? *((uint32_t *)v.n) : 0;
    }

    operator     int64_t() {
        var &v = resolve(*this); // todo: why was this missing prior?
        if (v.t == Type::Ref)
            return int64_t(*v.n_value.vref);
        return v.n ? *(( int64_t *)v.n) : 0;
    }

    operator    uint64_t() {
        var &v = resolve(*this);
        if (v.t == Type::Ref)
            return uint64_t(*v.n_value.vref);
        return v.n ? *((uint64_t *)v.n) : 0;
    }

    operator       void*() {
        var &v = resolve(*this);
        assert(v.t == Type::Ref);
        return v.n;
    }

    operator       node*() {
        var &v = resolve(*this);
        assert(v.t == Type::Ref);
        return (node*)v.n;
    }

    operator       float() {
        var &v = resolve(*this);
        if (v.t == Type::Ref) return float(*v.n_value.vref);
        assert( v.t == Type::f32 || v.t == Type::f64);
        return (v.t == Type::f32) ? *(float *)v.n :
               (v.t == Type::f64) ?   float(*(double *)v.n) :
               (v.t == Type::Str) ?   float(v.s.real()) : 0.0f;
    }

    operator      double() {
        var &v = resolve(*this);
        if (v.t == Type::Ref) return double(*v.n_value.vref);
        return (v.t == Type::f64) ? *(double *)v.n :
               (v.t == Type::f32) ?   double(*(float *)v.n) :
               (v.t == Type::Str) ? v.s.real() : 0.0;
    }
           operator string();
           operator bool();
    
    bool operator!();
    
    template <typename T>
    T *data() {
        var &v = resolve(*this);
        //v.compact();
        return (T *)v.d.get();
    }
    
       void reserve(size_t sz);
       Size    size();
       void    copy(var &ref);
       var     copy();
      
               ~var();
                var(const var &ref);
    
    bool operator!=(const var &ref);
    bool operator==(const var &ref);
    bool operator==(var &ref);
    var& operator= (const var &ref);
    bool operator==(::map<string, var> &m);
    bool operator!=(::map<string, var> &m);
    bool operator==(uint16_t v);
    bool operator==( int16_t v);
    bool operator==(uint32_t v);
    bool operator==( int32_t v);
    bool operator==(uint64_t v);
    bool operator==( int64_t v);
    bool operator==(   float v);
    bool operator==(  double v);
    void operator+=(     var d);
    
    static var parse_json(str &js);

    //template <typename T>
    //operator T *() {
    //    return data<T>();
    //}
        
    void notify_insert();
    void notify_delete();
    void notify_update();
    
    template <typename T>
    var& operator=(::array<T> aa) {
        m = null;
        t = Type::Array;
        c = Type::Any;
        Type::Specifier last_type = Type::Specifier(-1);
        int types = 0;
        a = ::array<var>(aa.size());
        for (auto &v: aa) {
            var d = var(v);
            if (d.t != last_type) {
                last_type  = Type::Specifier(size_t(d.t));
                types++;
            }
            a += v;
        }
        if (types == 1)
            c = last_type;
        
        notify_update();
        return *this;
    }
    
    template <typename T>
    var& operator=(::map<string, T> mm) {
        a = null;
        m = null;
        for (auto &[k, v]: mm)
            m[k] = v;
        notify_update();
        return *this;
    }
    
    var &operator=(Fn _fn) {
         t = Type::Lambda;
        fn = _fn;
        notify_update();
        return *this;
    }
    
    static str format(str t, ::array<var> p) {
        for (size_t k = 0; k < p.size(); k++) {
            str  p0 = str("{") + str(std::to_string(k)) + str("}");
            str  p1 = string(p[k]);
            t       = t.replace(p0, p1);
        }
        return t;
    }
};

/// Map is var:map, thats simple enough.
struct Map:var {
    ///
    typedef std::vector<pair<string,var>> vpairs;
    
    /// default
    Map(::map<string, var> m = {}) : var(m)  { }
    Map(nullptr_t n)               : var(::map<string, var>()) { }
    
    /// map from args
    Map(int argc, cchar_t* argv[], Map def = {}, string field_field = "");
    
    /// map from initializer list
    Map(std::initializer_list<pair<string, var>> args) : var(::map<string, var>()) {
        m = ::map<string, var>(args.size());
        for (auto &[k,v]: args)
            m[k ] = v;
    }
    
    /// assignment op and copy constructor
    Map(const Map &m) :  var(m.m) { }
    Map(const var &m) :  var(m.m) { }
    Map &operator=(const Map &ref) {
        if (this != &ref) {
            t = ref.t;
            c = ref.c;
            m = ref.m;
        }
        return *this;
    }
   ~Map() { }

    ///
    inline var              &back() { return m.pairs->back().value; }
    inline var             &front() { return m.pairs->front().value; }
    inline vpairs::iterator begin() { return m.begin(); }
    inline vpairs::iterator   end() { return m.end();   }
};

/// we need the distinction that a var is a Ref type on its type so member specialization can happen, keeping the internals of var.
struct Ref:var {
    Ref():var(Type::Ref) { }
    Ref(var &v) {
        t = Type::Ref;
        n_value.vref = &resolve(v);
    }
};





template <typename T>
static T input() {
    T v;
    std::cin >> v;
    return v;
}

enum LogType {
    Dissolvable,
    Always
};

/// could have a kind of Enum for LogType and LogDissolve, if fancied.
typedef std::function<void(str &)> FnPrint;

template <const LogType L>
struct Logger {
    FnPrint printer;
    Logger(FnPrint printer = null) : printer(printer) { }
    ///
protected:
    void intern(str &t, array<var> &a, bool err) {
        t = var::format(t, a);
        if (printer != null)
            printer(t);
        else {
            auto &out = err ? std::cout : std::cerr;
            out << std::string(t) << std::endl << std::flush;
        }
    }

public:
    
    /// print always prints
    inline void print(str t, array<var> a = {}) { intern(t, a, false); }

    /// log categorically as error; do not quit
    inline void error(str t, array<var> a = {}) { intern(t, a, true); }

    /// log when debugging or LogType:Always, or we are adapting our own logger
    inline void log(str t, array<var> a = {}) {
        if (L == Always || printer || is_debug())
            intern(t, a, false);
    }

    /// assertion test with log out upon error
    inline void assertion(bool a0, str t, array<var> a = {}) {
        if (!a0) {
            intern(t, a, false); /// todo: ion:dx environment
            assert(a0);
        }
    }

    /// log error, and quit
    inline void fault(str t, array<var> a = {}) {
        _print(t, a, true);
        assert(false);
        exit(1);
    }

    /// prompt for any type of data
    template <typename T>
    inline T prompt(str t, array<var> a = {}) {
        _print(t, a, false);
        return input<T>();
    }
};

extern Logger<LogType::Dissolvable> console;

///
}