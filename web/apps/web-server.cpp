#include <web/web.hpp>
#include <ux/ux.hpp>

struct Animals:node {
    declare(Animals);
    Element render() {
        Data &animals = context(node::props.bind);
        return Element::each(animals, [&](str s) -> Data { return s; });
    }
};

struct ExampleServer:node {
    declare(ExampleServer);
    void mount() {
        data = Data(Data::Map);
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

// so you render based on the client; thats fairly simple
Args defaults = {
    {"port", int(443)}
};

int main(int c, const char *v[]) {
    return App::Server(c, v, defaults)([&](Args &args) {
        return ExampleServer(args);
    });
}
