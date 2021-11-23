#include <web/web.hpp>
#include <ux/ux.hpp>

/// grab a json resource into var form
/// when debugging, set cwd to res so the web module sees the https trust certs
int main(int argc, const char *argv[]) {
    var  defs = Map {
        { "url", "https://ar-visions.github.io/test-resource.json" }
    };
    Args args = var::args(argc, argv, defs, "url");
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
