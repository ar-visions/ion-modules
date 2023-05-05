#pragma once

typedef map<str, var> Schema;
typedef map<str, var> SMap;
typedef array<var>     Table;
typedef map<str, var>  ModelMap;

inline var ModelDef(str name, Schema schema) {
    var m   = schema;
        m.s = name;
    return m;
}

/// re-integrate!
struct Remote {
        int64_t value;
            str remote;
    Remote(int64_t value)                  : value(value)                 { }
    Remote(std::nullptr_t n = nullptr)          : value(-1)                    { }
    Remote(str remote, int64_t value = -1) : value(value), remote(remote) { }
    operator int64_t() { return value; }
    operator var() {
        if (remote) {
            var m = { Type::Map, Type::Undefined }; /// invalid state used as trigger.
                m.s = string(remote);
            m["resolves"] = var(remote);
            return m;
        }
        return var(value);
    }
};

struct Ident   : Remote {
       Ident() : Remote(null) { }
};

struct node;
struct Raw {
    std::vector<std::string> fields;
    std::vector<std::string> values;
};

typedef std::vector<Raw>                        Results;
typedef std::unordered_map<std::string, var>    Idents;
typedef std::unordered_map<std::string, Idents> ModelIdents;

struct ModelCache {
    Results       results;
    Idents        idents; /// lookup fmap needed for optimization
};

struct ModelContext {
    ModelMap      models;
    std::unordered_map<std::string, std::string> first_fields;
    std::unordered_map<std::string, ModelCache *> mcache;
    var           data;
};

struct Adapter { // this was named DX, it needs a different name than this, though.  DX is the API end of an app, its data experience or us
    ModelContext &ctx;
    str           uri;
    Adapter(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
    virtual var record(var &view, int64_t primary_key  = -1) = 0;
    virtual var lookup(var &view, int64_t primary_key) = 0;
    virtual bool reset(var &view) = 0;
};


struct SQLite:Adapter {
    SQLite(ModelContext &ctx, str uri) : Adapter(ctx, uri) { fetch(); }
    var record(var &view, int64_t primary_key = -1);
    var lookup(var &view, int64_t primary_key);
    bool reset(var &view);
    var &operator[](str table_name) {
        assert(ctx.data.count(table_name));
        return ctx.data[table_name];
    }
    protected:
    void fetch();
    void resolve(str table_name);
    void observe(str table_name);
};
