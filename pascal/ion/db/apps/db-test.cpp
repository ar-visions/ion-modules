#include <db/db.hpp>

/// implicit data-binding used here
int main(int argc, char **argv) {
    /// define models
    ///
    ModelContext ctx;
    ctx.models["A"] = Schema("A", {
        { "a_0", Ident() }, /// merging Resolves and ModelKey is just the way to go.
        { "a_1",   str("") }
    });
    ///
    ctx.models["B"] = Schema("B", {
        { "b_0",        Ident() },
        { "b_1",       Remote("A") },
        { "b_2",          str("") }
    });
    /// create table for A
    /// records have no bindings in place yet, not until they're added
    SQLite db   = { ctx, "data/db.sqlite" };
    var A       = db["A"];
    var a       = db.record(A);
    a["a_1"]    = str("abc");
    A          += a;
    ///
    console.log("resultant primary key: {0}", {a["a_0"]}); /// a -> string should be primary key field in a logging context, but not in a json context
    var B       = db["B"];
    ///
    var a_found =
        A.select_first([&](var &a) -> var {
            if (a["a_1"] == var { str("abc") }) { /// support <, >, >=, <=
                console.log("a row exists");
                return &a;
            }
            return null;
        });
    ///
    if (a_found) {
        var b = db.record(B);
        b["a_id"] = &a_found; /// this can auto-ref on assignment (and likely should if its read only coming in -- which it needs to be..) but this is simple enough
        console.log("satellite primary key {0}", {b["a_id"]["a_0"]});
        B += b;
    }
    return 0;
}
