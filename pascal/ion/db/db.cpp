#include <core/core.hpp>
#include <db/db.hpp>
#include <stdio.h>
#include <sqlite3.h>

typedef int (*sql_rowfn)(void *, int, char **, char **);

var SQLite::lookup(var &view, int64_t primary_key) {
    str pk_field = ctx.first_fields[view];
    return view.select_first([&](var &r) {
        return (r[pk_field] == primary_key) ? r : var(null);
    });
}

/// reference to existing record, or new record with optional explicit key
var SQLite::record(var &view, int64_t primary_key) {
    var      row = view["model"].copy();
    var     &ref = view["refs"];
    str   m_name = view["model"];
    str pk_field = ctx.first_fields[view];
    ///
    if (primary_key != -1) {
        var found = lookup(view, primary_key);
        if (found) {
            /// return ref to this existing, resolved record
            var &v = var::resolve(found);
            return var(&v);
        }
    }
    /// set fields on all keyed resources
    for (auto &[model_name, fields]: ref.map())
        for (var &field: *ref[model_name].a) {
            str s_field  = field;
            row[s_field] = var {
                int64_t(s_field == pk_field ? primary_key : -1), var::ReadOnly
            };
        }
    
    return row;
}

static str sqtype_from_var(var &v) {
    switch (v.t) {
        case var::   i8: return "INTEGER";
        case var::  ui8: return "INTEGER";
        case var::  i16: return "INTEGER";
        case var:: ui16: return "INTEGER";
        case var::  i32: return "INTEGER";
        case var:: ui32: return "INTEGER";
        case var::  i64: return "INTEGER";
        case var:: ui64: return "INTEGER";
        case var::  Ref: return "INTEGER";
        case var:: Bool: return "INTEGER";
        case var::  f32: return "REAL";
        case var::  f64: return "REAL";
        case var::  Str: return "TEXT NOT NULL";
        case var::  Map: return "INTEGER";
        default:
            break;
    }
    assert(false);
    return "";
}

static bool create(sqlite3 *db, str table_name, var &model) {
    char       *err = null;
    const size_t sz = model.size();
    string   fields = string(1024);
    int       index = 0;
    ///
    for (auto &[k,v]: model.map()) {
        string field_name = k;
        bool      is_last = index   == sz - 1;
        bool    primary_k = index++ == 0;
        string     sqtype = sqtype_from_var(v);
        fields           += "\t" + field_name + " " + sqtype;
        if (primary_k)
            fields       += " PRIMARY KEY";
        if (!is_last)
            fields       += ",\n";
    }
    assert(index > 0);
    string q = "CREATE TABLE IF NOT EXISTS " + table_name + " (\n" + fields + ")";
    console.log(q);
    int rc  = sqlite3_exec(db, q.c_str(), null, null, &err);
    if (rc != SQLITE_OK) {
        console.error("create: SQL error: {0}\n", {str(err)});
        sqlite3_free(err);
    }
    return rc == SQLITE_OK;
}

static inline str sql_encode(str s) {
    return s.replace("\\","\\\\").replace("'", "\'");
}

static str model_key(var &model) {
    /// get the primary key (always the first field)
    str kname = null;
    for (auto &[k,v]: model.map()) {
        kname = k;
        assert(v == var::i64);
        break;
    }
    return kname;
}

static int process_raw(ModelCache &cache, int count, char **field_values, char **field_names) {
    Raw raw = {};
    for (int i = 0; i < count; i++) {
        raw.fields.push_back(field_names[i]);
        raw.values.push_back(field_values[i]);
    }
    cache.results.push_back(raw);
    return 0;
}

/// resolve fields
bool SQLite::reset(var &view) {
    string table_name = view;
    sqlite3  *db = null;
    char    *err = null;
    int       rc = sqlite3_open(uri.cstr(), &db);
    str       qy = var::format("DROP TABLE IF EXISTS {0}", {table_name});
    ///
    console.log("reset: {0}", {qy});
    rc = sqlite3_exec(db, qy.cstr(), null, null, &err);
    if (rc != SQLITE_OK) {
        console.error("destroy: SQL error: %s\n", {str(err)});
        sqlite3_free(err);
        return false;
    }
    return true;
}

/// resolve fields
void SQLite::resolve(str model_name) {
    var         table = &ctx.data[model_name];
    ModelCache &cache = *ctx.mcache[model_name];
    ///
    for (auto &raw: cache.results) {
        var rec = record(table);
        int  n_field = raw.fields.size();
        for (int i = 0; i < n_field; i++) {
            string vstr = raw.values[i];
            /// if its an id, we will lookup
            var &v = rec[raw.fields[i]];
                 v = var { v.t, vstr };
        }
        table += rec;
    }
}
///
/// returns implicitly-bound var table
/// records added to and removed from as well as field value changes
/// (foreign references included) are managed here with var::observe
///
void SQLite::observe(str model_name) {
    ctx.data[model_name].observe( [ctx=&ctx, uri=uri, model_name=model_name]
                   (var::Binding op, var &row, str &field, var &value) {
        ///
        const str UPDATE_QY = "UPDATE {0} SET {1} = {2} WHERE {3} = {4}";
        const str INSERT_QY = "INSERT INTO {0} ({1}) VALUES ({2})";
        const str DELETE_QY = "DELETE FROM {0} WHERE {1} = {2}";
        ///
        var    model = ctx->models[model_name];
        str    kname = model_key(model);
        sqlite3  *db = null;
        char    *err = 0;
        str       qy = null;
        int       rc = sqlite3_open(uri.cstr(), &db);
        bool key_req = op == var::Binding::Update || op == var::Binding::Delete;
        ///
        assert(!key_req || row.count(kname)); /// if the key is required, it needs to exist on the row
        ///
        int64_t ikey = key_req ? int64_t(row[kname])  : int64_t(-1);
        str     skey = key_req ? str(std::to_string(ikey)) : str(null);
        var    &refs = ctx->data[model_name]["refs"];
        ///
        assert(rc == SQLITE_OK);
        ///
        auto str_from_value = [&](str field, var &value) {
            if (value == Type::Str)
                return var::format("'{0}'", {sql_encode(value)});
            ///
            if (value == Type::Map) {
                for (auto &[peer_name, v_fields]: refs.map()) {
                    string speer_name = peer_name;
                    var field_names_used = v_fields;
                    if (refs.count(peer_name) && refs[peer_name].count(field)) {
                        auto peer_key = ctx->first_fields[peer_name];
                        return value ? str(value[peer_key]) : str("NULL");
                    }
                }
                console.fault("unsupported value: {0}", {field}); /// unsupported value
            }
            return str(value);
        };
        ///
        switch (op) {
            case var::Binding::Update: {
                str sv = str_from_value(field, value);
                qy = var::format(UPDATE_QY, {model_name, field, sv, kname, skey});
                break;
            }
            case var::Binding::Insert: {
                str   fields = "";
                str   values = "";
                bool   first = true;
                for (auto &[field, v]: model.map()) {
                    /// automatic / explicit key handling
                    str  explicit_key = (first && row.count(field) && row[field] == var::i64) ?
                                            str(int64_t(row[field])) : str(null);
                    if (first && !explicit_key) {
                        first = false;
                        continue;
                    }
                    /// directed by schema fmap, not the row
                    if (fields) {
                        fields += ", ";
                        values += ", ";
                    }
                    auto s_field = field;
                    var    &rval = var::resolve(row[s_field]);
                    fields      += field;
                    values      += explicit_key ? explicit_key : str_from_value(field, rval);
                    first        = false;
                }
                qy = var::format(INSERT_QY, { model_name, fields, values });
                break;
            }
            case var::Binding::Delete: {
                qy = var::format(DELETE_QY, { model_name, kname, skey });
                break;
            }
        }
        console.log("observe: {0}", {qy});
        rc = sqlite3_exec(db, qy.cstr(), null, null, &err);
        ///
        /// set primary key on insert
        if (op == var::Binding::Insert) {
            int64_t id = sqlite3_last_insert_rowid(db);
            row[kname] = id;
        }
        sqlite3_close(db);
    });
}

///
/// fetch data for all models, resolve and then observe
void SQLite::fetch() {
    for (auto &[k,v]: ctx.mcache)
        delete v;
    ///
    ctx.mcache = {};
    for (auto &[model_name, model]: ctx.models) {
        ModelCache &cache = *(ctx.mcache[model_name] = new ModelCache);
        /// open db and query table
        string       m = str(model);
        sqlite3    *db = null;
        int         rc = sqlite3_open(uri.cstr(), &db);
        assert(     rc == SQLITE_OK);
        char      *err = 0;
        ctx.data[m]    = var {Type::Array, Type::Map};
        var     &table = ctx.data[m]; /// creates data for table
        table["model"] = &model;
        table["refs"]  = { Type::Map };
        ///
        /// use this data for the queries in a bit
        bool first = true;
        for (auto &[field, value]: model.map())
            if (first || value.count("resolves")) {
                str model_name = first ? str(model) : str(value["resolves"]); // model name
                if (!table["refs"][model_name])
                     table["refs"][model_name] = { Type::Array, Type::Str };
                table["refs"][model_name] += field;
                if (first)
                    ctx.first_fields[m] = field;
                first = false;
            }
        str         qy = var::format("SELECT * FROM {0}", {m});
                    rc = sqlite3_exec(db, qy.cstr(), sql_rowfn(process_raw),
                                      (void *)&cache, &err);
        /// create sql table on failure of select
        if (rc != SQLITE_OK) {
            bool success = create(db, m, model);
            assert(success);
        }
        ///
        sqlite3_close(db);
    }
    ///
    for (auto &[model_name, model]: ctx.models)
        resolve(model_name);
    ///
    /// observe after. the bindings are broadcast throughout row and field values by observe
    for (auto &[model_name, model]: ctx.models)
        observe(model_name);
}

