#include <core/core.hpp>

logger console;

///
int str::index_of(MatchType ct, symbol mp) const {
    int index = 0;

    using Fn = func<bool(const char &)>;
    static umap<MatchType, Fn> match_table {
        { Alpha,     Fn([&](const char &c) -> bool { return  isalpha (c); }) },
        { Numeric,   Fn([&](const char &c) -> bool { return  isdigit (c); }) },
        { WS,        Fn([&](const char &c) -> bool { return  isspace (c); }) }, // lambda must used copy-constructed syntax
        { Printable, Fn([&](const char &c) -> bool { return !isspace (c); }) },
        { String,    Fn([&](const char &c) -> bool { return  strcmp  (&c, mp) == 0; }) },
        { CIString,  Fn([&](const char &c) -> bool { return  strcmp  (&c, mp) == 0; }) }
    };

    /// msvc thinks its ambiguous, so i am removing this iterator from str atm.
    cstr pc = data;
    for (;;) {
        char &c = pc[index];
        if  (!c)
            break;
        if (match_table[ct](c))
            return index;
        index++;
    }
    return -1;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    int n;
    va_list args;
    va_start(args, format);

#ifdef _MSC_VER
    n = _vsnprintf_s(str, size, _TRUNCATE, format, args);
#else
    n = vsnprintf(str, size, format, args);
#endif

    va_end(args);
    if (n < 0 || n >= (int)size) {
        // handle error here
    }
    return n;
}

str path::mime_type() {
    mx e = ext().symbolize();
    static path  data = "data/mime-type.json";
    static map<mx> js =  data.read<var>();
    return js.count(e) ? ((memory*)js[e])->grab() : ((memory*)js["default"])->grab();
}

i64 integer_value(memory *mem) {
    symbol   c = mdata<char>(mem, 0);
    bool blank = c[0] == 0;
    while (isalpha(*c))
        c++;
    return blank ? i64(0) : i64(strtol(c, nullptr, 10));
}

memory *drop(memory *mem) {
    if (mem) mem->drop(); 
    return mem;
}

memory *grab(memory *mem) {
    if (mem) mem->grab();
    return mem;
}

i64 millis() {
    return i64(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );
}

/// attach arbs to memory (uses a pointer)
attachment *memory::attach(::symbol id, void *data, lambda<void()> release) {
    if (!atts)
        atts = new doubly<attachment>();
    atts->push(attachment {id, data, release});
    return &atts->last();
}

attachment *memory::find_attachment(::symbol id) {
    if (!atts) return nullptr;
    /// const char * symbol should work fine for the cases used
    for (attachment &att:*atts)
        if (id == att.id)
            return &att;
    return nullptr;
}

memory *memory::stringify(cstr cs, size_t len, size_t rsv, bool constant, type_t ctype, i64 id) {
    ::symbol sym = (::symbol)(cs ? cs : "");
    if (constant) {
        if(!ctype->symbols)
            ctype->symbols = new symbol_data { };
        u64  h_sym = djb2(cstr(sym));
        memory *&m = ctype->symbols->djb2[h_sym];
        if (!m) {
            size_t ln = (len == memory::auto_len) ? strlen(sym) : len; /// like auto-wash
            m = memory::alloc(typeof(char), typesize(char), len, len + 1, raw_t(sym));
            m->attrs = attr::constant;
            ctype->symbols->ids[id] = m; /// was not hashing by id, it was the djb2 again (incorrect)
            //ctype->symbol_djb2[h_sym] = m; 'redundant due to the setting of the memory*& (which [] operator always inserts item)
            ctype->symbols->list.push(m);
        }
        return m->grab();
    } else {
        size_t     ln = (len == memory::auto_len) ? strlen(sym) : len;
        size_t     al = rsv ? rsv : (strlen(sym) + 1);
        memory*   mem = memory::alloc(typeof(char), typesize(char), ln, al, raw_t(sym));
        cstr  start   = mem->data<char>(0);
        start[ln]     = 0;
        return mem; 
    }
}

memory *memory::string (std::string s) { return stringify(cstr(s.c_str()), s.length(), 0, false, typeof(char), 0); }
memory *memory::cstring(cstr s)        { return stringify(cstr(s),         strlen(s),  0, false, typeof(char), 0); }

memory *memory::symbol (::symbol s, type_t ty, i64 id) {
    return stringify(cstr(s), strlen(s), 0, true, ty, id);
}

memory *memory::raw_alloc(type_t type, size_t sz, size_t count, size_t res) {
    size_t elements = math::max(count, res);
    memory*     mem = (memory*)calloc(1, sizeof(memory) + elements * sz); /// there was a 16 multiplier prior.  todo: add address sanitizer support with appropriate clang stuff
    mem->count      = count;
    mem->reserve    = res;
    mem->refs       = 1;
    mem->type       = type;
    mem->origin     = (void*)&mem[1];
    return mem;
}

memory *memory::wrap(raw_t m, type_t ty) {
    memory*     mem = (memory*)calloc(1, sizeof(memory)); /// there was a 16 multiplier prior.  todo: add address sanitizer support with appropriate clang stuff
    mem->count      = 1;
    mem->reserve    = 1;
    mem->refs       = 1;
    mem->type       = ty;
    mem->origin     = m;
    return mem;
}   

memory::memory() : refs(1) { }


/// starting at 1, it should remain active.  shall not be freed as a result
static memory m_null = { 1, u64(0), typeof(mx), 0, 0, null, 0, null };

void memory::drop() {
    if (--refs <= 0 && !constant) { /// <= because ptr does a defer on the actual construction of the container class
        assert(this != &m_null);
        /// call destructor on memory payload, if defined
        if (type->lambdas->dtr)
            type->lambdas->dtr(type, origin, 0, count);
        if (origin != (this + 1)) {
            //free(origin);
            origin = null;
        }
        // delete attachment lambda after calling it
        if (atts) {
            for (attachment &att: *atts)
                att.release();
            delete atts;
            atts = null;
        }
        if (shape) {
            delete shape;
            shape = null;
        }
        //free(this);
    }
}

/// now we start allocating the total_size (or type->base_sz if not schema-bound (mx classes accept schema.  they say, icanfly.. imapilot))
memory *memory::alloc(type_t type, size_t type_sz, size_t count, size_t reserve, raw_t src_origin, bool call_ctr) {
    static int abc = 0; abc++;
    // build times can be tracked with metrics
    if (abc == 42) { // type_sz should not be sizeof(MX) it needs to remain typesize()
        int test = 0;
        test++;
    }
    /// must specify a type_sz if there is a copy operation
    assert(!src_origin || type_sz);

    /// requirement: array of mx is just array of memory*
    /// mx = memory* size
    if (type_sz == 0)
        type_sz = type->schema ? type->schema->total_bytes : type->base_sz;
    
    memory  *mem  = null;

    /// singletons are fun as 1:1 class-instanced data
    bool singleton = type->traits & traits::singleton;
    if  (singleton && type->singleton) return type->singleton->grab();

    /// memory::alloc with a type of mx has no data-type payload
    /// when doing default allocation of mx, this mx type arg is passed and routes to m_null
    /// there isnt much of a point to allocate multiple stateless objects however there could be attrib differences
    /// as a simple solution it seems best to tokenize rather than return null
    if (!type_sz) {
        mem = &m_null;
        mem->refs++; /// token or not, use its refs.  otherwise you need to check that during life-cycle
    } else {
        mem = raw_alloc(type, type_sz, count, reserve);
    }

    /// if allocating a schema-based object (mx being first user of this)
    if (type->schema && type->schema->total_bytes) {
        assert(type_sz == type->schema->total_bytes);
        ///
        if (call_ctr) {
            if (src_origin) {
                /// if schema-copy-construct (call cpctr for each data type in composition)
                for (size_t i = 0; i < type->schema->count; i++) {
                    context_bind &bind = type->schema->composition[i];
                    if (bind.data) bind.data->lambdas->cp(bind.data, mem->origin, src_origin, 0, count);
                }
            } else {
                /// ctr: call construct across the composition
                for (size_t i = 0; i < type->schema->count; i++) {
                    context_bind &bind = type->schema->composition[i];
                    if (bind.data) bind.data->lambdas->construct(bind.data, mem->origin, 0, count); /// probably good to avoid the context bind at all if there is no data; in some cases it may not be required and the context does offer some insight by itself
                }
            }
        }
    } else {
        if (call_ctr) {
            const bool prim = (type->traits & traits::primitive);
            if (src_origin)
                type->lambdas->cp(type, mem->origin, src_origin, 0, count);
            else if (!prim) /// no need. tis zero.
                type->lambdas->construct(type, mem->origin, 0, count);
        }
    }
    if (singleton) type->singleton = mem->grab(); /// user we return to is a user (+1 at alloc), this is a user as well. (==2)
    return mem;
}

memory *memory::copy(size_t reserve) {
    memory *a   = this;
    memory *res = alloc(a->type, a->type_size(), a->count, reserve, a->origin);
    return  res;
}

memory *memory::grab() {
    refs++;
    return this;
}

size &size::operator=(const size b) {
    memcpy(values, b.values, sizeof(values));
    count = b.count;
    return *this;
}


void chdir(std::string c) {
    #if defined(_WIN32)
    // replace w global for both cases; its not worth it
    //SetCurrentDirectory(c.c_str());
    #else
    ::chdir(c.c_str());
    #endif
}

memory* mem_symbol(::symbol cs, type_t ty, i64 id) {
    return memory::symbol(cs, ty, id);
}

memory* raw_alloc(idata *ty, size_t sz) {
    return memory::raw_alloc(ty, sz);
}

void *mem_origin(memory *mem) {
    return mem->origin;
}

memory *cstring(cstr cs, size_t len, size_t reserve, bool is_constant) {
    return memory::stringify(cs, len, 0, is_constant, typeof(char), 0);
}


void shared::drop() {
    if (mem)
        mem->drop();
}

memory *shared::grab() {
    return mem ? mem->grab() : null;
}

shared::~shared() {
    if (mem)
        mem->drop();
}

shared::shared(const shared &ref) : ident(((shared &)ref).grab()) { }
