#include <web/web.hpp>
#include <ux/ux.hpp>

/// Development In progress...
struct Animals:node {
    declare(Animals);
    Element render() {
        var &animals = context(node::props.bind);
        return Element::each(animals, [&](str s) -> var { return s; });
    }
};

struct ExampleServer:node {
    declare(ExampleServer);
    void mount() {
        data = var(var::Map);
        vec<str> s = {"chicken", "frog", "bird"};
        data["animals"] = s;
    }
    Element render() {
        return Animals {{
            {"id",   "amazing-resource"},
            {"bind", "animals"}
        }};
    }
};

Args defaults = {
    {"port", int(443)}
};

int main(int c, const char *v[]) {
    return App::Server(c, v, defaults)([&](Args &args) {
        return ExampleServer(args);
    });
}
