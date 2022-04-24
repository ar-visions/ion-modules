export module model;

import io;
import map;
import str;
import arr; // var depends on arr
import var; // arr depends on var

export typedef map<str, var> Schema;
export typedef map<str, var> SMap;
export typedef array<var>     Table;
export typedef map<str, var>  ModelMap;

export var ModelDef(str name, Schema schema) {
    var m   = schema;
        m.s = name;
    return m;
}

export struct Remote {
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

export struct Ident : Remote {
       Ident() : Remote(null) { }
};

struct node;
export struct Raw {
    std::vector<std::string> fields;
    std::vector<std::string> values;
};

export typedef std::vector<Raw>                        Results;
export typedef std::unordered_map<std::string, var>    Idents;
export typedef std::unordered_map<std::string, Idents> ModelIdents;

export struct ModelCache {
    Results       results;
    Idents        idents; /// lookup fmap needed for optimization
};

export struct ModelContext {
    ModelMap      models;
    std::unordered_map<std::string, std::string> first_fields;
    std::unordered_map<std::string, ModelCache *> mcache;
    var           data;
};

export struct Adapter {
    ModelContext &ctx;
    str           uri;
    Adapter(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
    virtual var record(var &view, int64_t primary_key  = -1) = 0;
    virtual var lookup(var &view, int64_t primary_key) = 0;
    virtual bool reset(var &view) = 0;
};
