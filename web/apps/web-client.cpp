#include <web/web.hpp>
#include <ux/ux.hpp>

/// fetch a json resource, with quotes from anton petrov
int main(int c, const char *v[]) {
    var  defs = Map {
        { "url", "https://ar-visions.github.io/wonderful-resource.json" }
    };
    Args args = var::args(c, v, defs, "url");
    ///
    Web::json(args["url"]).then([&](var &v_msg) {
        Web::Message msg = v_msg;
        bool b_value = msg.content["stay-wonderful"];
        if (b_value)
            console.log("expected result: {0}", {b_value});
        else
            console.fatal("space out");
        ///
    }).except([&](var &err) {
        console.log("request failure: {0}", {err});
    });
    ///
    return Async::await();
}
