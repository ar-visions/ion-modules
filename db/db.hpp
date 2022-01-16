#pragma once
#include <dx/dx.hpp>

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
