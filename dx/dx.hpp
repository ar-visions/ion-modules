#pragma once
#include <dx/var.hpp>

struct Raw {
    std::vector<std::string> fields;
    std::vector<std::string> values;
};

typedef std::vector<Raw>                Results;
typedef std::map<std::string, var>      Idents;
typedef std::map<std::string, Idents>   ModelIdents;

struct ModelCache {
    Results       results;
    Idents        idents; /// lookup map needed for optimization
};

struct ModelContext {
    ModelMap      models;
    std::map<std::string, std::string> first_fields;
    std::map<std::string, ModelCache *> mcache;
    var           data;
};

struct DX {
    ModelContext &ctx;
    str           uri;
    DX(ModelContext &ctx, str uri) : ctx(ctx), uri(uri) { }
    virtual var record(var &view, int64_t primary_key = -1) = 0;
    virtual var lookup(var &view, int64_t primary_key) = 0;
    virtual bool reset(var &view) = 0;
};
