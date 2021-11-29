#include <ux/ux.hpp>
#include <ux/app.hpp>

struct Sample:node {
    declare(Sample);
    
    struct Props:IProps {
        str test;
    } props;
    
    void define() {
        Define <str> {this, "test", &props.test, "hello vulkan, hello skia."};
    }
    
    Element render() {
        return Label {{
                {"text-label",   "hello vulkan, skia, and whole world of components."}, /// label out, letter go..
                {"text-align-x",  Align::Start},
                {"region",        Region {L{1}, T{1}, W{1.0}, H{1.0}}}
            }};
    }
};

Args defaults = {
    {"window-width",  int(1024) },
    {"window-height", int(1024)}
};

int main(int c, const char *v[]) {
    return App::UX(c, v, defaults)([&] (Args &args) {
        return Sample(args);
    });
}
