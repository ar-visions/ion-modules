#include <ux/ux.hpp>

static Map def = {
    {"gravity", 1.0}
};

struct Car:node {
    declare(Car);
    ///
    struct Model {
        enum Type {
            Venkman,
            Egon,
            Winston,
            Ray,
            Dayna,
            Zuul
        };
        Type   type;
        str    name;
        float  max_speed;
        bool   active;
        ///
        static array<Model> models;
        ///
        static void load() {
            if (models)
                return;
            ///
            models  = array<Model>(size_t(Zuul) + 1);
            models += Model { Venkman,  str("Venkman"), 1.0f, true };
            models += Model { Egon,     str("Egon"),    1.0f, true };
            models += Model { Winston,  str("Winston"), 1.0f, true };
            models += Model { Ray,      str("Ray"),     1.0f, true };
            models += Model { Dayna,    str("Dayna"),   1.0f, true };
            models += Model { Zuul,     str("Zuul"),    1.0f, true };
        }
    };
    ///
    Model::Type type;
    float       health;
    vec3f       pos;
};

struct Entity {
    void draw(Canvas &c) {
    }
};

struct Platform:Entity {
    Style style() {
        return {
            {"*", {{"model", "platform.obj"}}}
        };
    }
};

struct CarSelection:View {
    struct Members {
        float gravity;
    };
    
    void mount() {
        Car::Model::load();
    }
    
    Element dialog() {
        return null;
    }
    
    Element render() {
        return Element::each(cars, [&](Car &c) {
            return Platform {{
                
            }};
        });
    }
};

int main(int c, char *v[]) {
    return Window(c, v, def)([&](Map &args) { return CarSelection(args); });
}
