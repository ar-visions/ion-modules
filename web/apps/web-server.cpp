// The idea here is to use the same Component reflection model to perform server-related tasks.  One form of reflection is used with UX, the other with 'DX'..

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
        data = var(Type::Map);
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
    return Server(c, v, defaults)([&](Args &args) {
        return ExampleServer(args);
    });
}
