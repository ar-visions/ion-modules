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
        data            = var(Type::Map);
        array<str>    s = {"chicken", "frog", "bird"};
        data["animals"] = s;
    }
    Element render() {
        return Animals {{
            {"id",   "amazing-resource"},
            {"bind", "animals"}
        }};
    }
};

Map defaults = {
    {"port", int(443)}
};

int main(int c, cchar_t *v[]) {
    return Server(c, v, defaults)([&](Map &args) {
        return ExampleServer(args);
    });
}
