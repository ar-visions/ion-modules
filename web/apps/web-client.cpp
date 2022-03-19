#include <web/web.hpp>

/// grab a json resource into var form
/// set cwd to res so the web module sees the https trust certs
int main(int argc, cchar_t *argv[]) {
    Map defs = {{ "url", "https://ar-visions.github.io/test-resource.json" }};
    Map args = var::args(argc, argv, defs, "url");
    ///
    Web::json(args["url"]).then([&](Message msg) {
        if (msg.content["wonderful"])
            console.log("expected result");
        ///
    }).except([&](var &err) {
        console.log("request failure: {0}", {err});
    });
    return async::await();
}
