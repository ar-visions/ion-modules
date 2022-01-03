#include <ux/node.hpp>
#include <ux/style.hpp>
#include <ux/node.hpp>
//#include <regex> in time.

map<path_t, Style> Style::cache            = {};
map<str, vec<ptr<StBlock>>> Style::members = {};

struct StBlock;

/// -------------------------------------------------
size_t    Style_members_count(str &s)              { return (Style::members.count(s)); }
vec<ptr<StBlock>> &Style_members(str &s)           { return Style::members[s]; }
double          StBlock_match(StBlock *b, node *n) { return b->match(n); }
StPair*         StBlock_pair (StBlock *b, str  &s) { return b->pair(s);  }
size_t StPair_instances_count(StPair *p, Type &t)  { return p->instances.count(t); }

StPair *Style_pair(Member *member) {
    vec<ptr<struct StBlock>> &blocks  = Style_members(member->name);
    struct StPair         *match      = null;
    double                 best_score = 0;
    ///
    /// find top style pair for this member
    for (auto ptr:blocks) {
        StBlock *block = ptr;
        if (member->name == "bg") {
            int test = 0;
            test++;
        }
        double score = block->match(member->node);
        if (score > best_score) {
            match = block->pair(member->name);
            best_score = score;
        }
    }
    return match;
}

static bool ws(char *&cursor) {
    auto is_cmt = [](const char *c) -> bool { return c[0] == '/' && c[1] == '*'; };
    while (isspace(*cursor) || is_cmt(cursor)) {
        while (isspace(*cursor))
            cursor++;
        if (is_cmt(cursor)) {
            char *f = strstr(cursor, "*/");
            cursor = f ? &f[2] : &cursor[strlen(cursor) - 1];
        }
    }
    return *cursor != 0;
}

static bool scan_to(char *&cursor, vec<char> chars) {
    bool sl = false;
    bool qt = false;
    bool qt2 = false;
    for (; *cursor; cursor++) {
        if (!sl) {
            if (*cursor == '"')
                qt = !qt;
            else if (*cursor == '\'')
                qt2 = !qt2;
        }
        sl = *cursor == '\\';
        if (!qt && !qt2)
            for (char &c: chars)
                if (*cursor == c)
                    return true;
    }
    return false;
}

static vec<StQualifier> parse_qualifiers(char **p) {
    str   qstr;
    char *start = *p;
    char *end   = null;
    char *scan  =  start;
    /// read ahead to {
    do {
        if (!*scan || *scan == '{') {
            end  = scan;
            qstr = str(start, std::distance(start, scan));
            break;
        }
    } while (*++scan);
    ///
    if (!qstr) {
        end = scan;
        *p  = end;
        return null;
    }
    ///
    auto  quals = qstr.split(",");
    auto result = vec<StQualifier>(quals.size());
    ///
    for (auto qs:quals) {
        str  q = qs.trim();
        if (!q)
            continue;
        StQualifier v = { };
        auto idot = q.index_of(".");
        auto icol = q.index_of(":");
        str tail;
        if (idot >= 0) {
            auto sp = q.split(".");
            v.type  = sp[0];
            v.id    = sp[1];
            if (icol >= 0)
                tail  = q.substr(icol + 1).trim();
        } else {
            if (icol  >= 0) {
                v.type = q.substr(0, icol);
                tail   = q.substr(icol + 1).trim();
            } else
                v.type = q;
        }
        auto   ops = vec<str> {"!=",">=","<=",">","<","="};
        if (tail) {
            // check for ops
            bool is_op = false;
            for (auto &op:ops) {
                if (tail.index_of(op.cstr()) >= 0) {
                    is_op   = true;
                    auto sp = tail.split(op);
                    v.state = sp[0].trim();
                    v.oper  = op;
                    v.value = tail.substr(sp[0].len() + op.len()).trim();
                    break;
                }
            }
            if (!is_op)
                v.state  = tail;
        }
        result += v;
    }
    *p = end;
    return result;
}

str parse_likely(char *v) {
    char *start = v;
    while (!isspace(*v++)) { }
    return str(start, std::distance(start, v));
}

void Style::cache_members() {
    std::function<void(ptr<StBlock> &b)> cache_b;
    cache_b = [&](ptr<StBlock> &b) -> void {
        for (auto &p:b->pairs) {
            bool found = false;
            vec<ptr<StBlock>> &cache = Style::members[p.member];
            if (p.member == "bg") {
                int test = 0;
                test++;
            }
            ///
            for (auto &cb:cache)
                found |= cb == b;
            if (!found)
                cache += b;
        }
        for (ptr<StBlock> &s:b->blocks)
            cache_b(s);
    };
    if (root)
        for (ptr<StBlock> &b:root)
            cache_b(b);
}

Style::Style(str &code) {
    ///
    if (code) {
        for (char *sc = (char *)code.cstr(); ws(sc); sc++) {
            std::function<void(ptr<StBlock> &)> parse_block;
            ///
            parse_block = [&](ptr<StBlock> &block) {
                ws(sc);
                console.assertion(*sc == '.' || isalpha(*sc), "expected Type, or .name");
                block->quals = parse_qualifiers(&sc);
                ws(++sc);
                ///
                while (*sc && *sc != '}') {
                    /// read up to ;, {, or }
                    ws(sc);
                    char *start = sc;
                    console.assertion(scan_to(sc, {';', '{', '}'}), "expected member expression or qualifier");
                    if (*sc == '{') {
                        /// lets blend the vector and the smart pointer.
                        /// 
                        block->blocks += ptr<StBlock>(new StBlock());
                        ptr<StBlock> &b = block->blocks[block->blocks.size() - 1];
                        b->parent = &block;
                        /// parse sub-block
                        sc = start;
                        parse_block(b);
                        assert(*sc == '}');
                        ws(++sc);
                        ///
                    } else if (*sc == ';') {
                        /// read member
                        char  *cur  = start;
                        console.assertion(scan_to(cur, {':'}) && (cur < sc), "expected [member:]value;");
                        str  member = str(start, std::distance(start, cur));
                        ws(++cur);

                        /// read value
                        char *vstart = cur;
                        console.assertion(scan_to(cur, {';'}), "expected member:[value;]");
                        str  value  = str(vstart, std::distance(vstart, cur)).trim();
                        if (value.substr(0, 1) == "\"" && value.substr(-1, 1) == "\"") {
                            char *cstr = (char *)value.cstr();
                            value = var::parse_quoted(&cstr, value.len());
                        }
                        
                        /// check
                        console.assertion(member, "member cannot be blank");
                        console.assertion(value,  "value cannot be blank");
                        block->pairs += StPair { member, value }; // this is going to realloc, so do it afterwards unless you want a disaster. disahsteh
                        ws(++sc);
                    }
                }
                console.assertion(!*sc || *sc == '}', "expected closed-brace");
            };
            ///
            root += ptr<StBlock>(new StBlock());
            /// a list or node container allocator pattern is superior to this one.
            /// you could even have dislike containers using it, they would use the same pointer protocol and data structure.
            /// for now i want to leave this as-is but i dont particularly enjoy the encapsulation for all of the items and vectors of them.
            /// templated typedefs would shim it a bit but with more names
            parse_block(root[root.size() - 1]);
        }
        /// store blocks by member, the interface into all style: Style::members[name]
        cache_members();
    }
}

void Style::unload() {
    console.log("zoooool.");
    members = {};
    cache   = {};
}

Style::~Style() {
    console.log("destructor.");
}

Style Style::load(path_t path) {
    if (cache.count(path) == 0) {
        str    code  = str::read_file(path);
        cache[path]  = Style(code);
    }
    return cache[path];
}

Style *Style::for_class(const char *class_name) {
    path_t p = str::format("style/{0}.css", { str(class_name) });
    return &(cache.count(p) ? cache[p] : (cache[p] = Style::load(p)));
}

size_t StBlock::score(node *n) {
    double best_sc = 0;
    for (auto &q:quals) {
        auto    &node_id  = n->element.id();
        bool    id_match  = q.id    && q.id == node_id;
        bool   id_reject  = q.id    && !id_match;
        bool  type_match  = q.type  && q.type == n->class_name;
        bool type_reject  = q.type  && !type_match;
        ///if (q.oper) {
        /// ---- support ops ---- just need to know if we are putting expressions in style (most likely)
        /// i think everything should be 'expression', if it has dynamic params then the type can tell us that
        ///}
        bool state_match  = q.state && n->state<bool>(q.state);
        bool state_reject = q.state && !state_match;
        ///
        if (!id_reject && !type_reject && !state_reject) {
            double sc = size_t(   id_match) << 2 |
                        size_t( type_match) << 1 |
                        size_t(state_match) << 0;
            best_sc = std::max(sc, best_sc);
        }
    }
    return best_sc;
};

/// each qualifier is defined as it, and all of the blocked qualifiers below.
/// there are more optimal data structures to use for this matching routine
/// state > state2 would be a nifty one, no operation indicates bool, as-is current normal syntax
double StBlock::match(node *n) {
    StBlock     *block = this;
    double total_score = 0;
    int            div = 0;
    while (block && n) {
        bool   is_root   = !block->parent;
        double score     = 0;
        ///
        while (n) { /// we have to get the best score for this block, and we are not.
            auto sc = block->score(n);
            n = n->parent;
            if (sc == 0 && is_root) {
                score = 0;
                break;
            } else if (sc > 0) {
                score += double(sc) / ++div;
                break;
            }
        }
        total_score += score;
        ///
        if (score > 0) {
            /// proceed...
            block = block->parent;
        } else {
            /// style not matched
            block = null;
            total_score = 0;
        }
    }
    return total_score;
}

void Style::init(path_t path) {
    resources(path, {".css"}, [&](path_t &p) -> void {
        for_class(p.stem().string().c_str());
    });
}
