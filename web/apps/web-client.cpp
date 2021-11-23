#include <web/web.hpp>
#include <ux/ux.hpp>

/// fetch a json resource, with quotes from anton petrov
/// to run, set cwd
int main(int c, const char *v[]) {
    var  defs = Map {
        { "url", "https://ar-visions.github.io/test-resource.json" }
    };
    Args args = var::args(c, v, defs, "url");
    ///
    Web::json(args["url"]).then([&](Web::Message msg) {
        if (msg.content["wonderful"])
            console.log("expected result");
        ///
    }).except([&](var &err) {
        console.log("request failure: {0}", {err});
    });
    ///
    return Async::await();
}
