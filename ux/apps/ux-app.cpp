#include <ux/ux.hpp>
#include <ux/app.hpp>

struct Selector:node {
    int member;
};

struct MultiSelector:node {
};


struct Commander:node {
    declare(Commander);
    
    struct Props:IProps {
        str path;
    } props;
    
    std::filesystem::path path;
    
    void define() {
        Define <str> {this, "path", &props.path, "."};
    }
    
    void mount() {
        update_resources();
    }
    
    void update_resources() {
        /// populate resources
        path = std::filesystem::path(props.path);
        size_t iter = 0;
        while (!std::filesystem::is_directory(path) || iter++ == 100)
            path = path.parent_path();
        data = Data(Data::Map);
        data["resources"] = {Data::Array};
        auto dir = std::filesystem::directory_iterator(path);
        for (auto &e: dir) {
            auto   ep = e.path();
            size_t sz = std::filesystem::is_regular_file(ep) ?
                        std::filesystem::file_size(ep)       : 0;
            auto  res = Args {{
                {"name", ep.stem().string()},
                {"ext",  ep.extension().string()},
                {"size", sz}
            }};
            data["resources"] += res;
        }
        data["resolve"] = (FnFilter) [&](Data &d) {
            return str(d);
        };
        data["edit-data"] = str("this is some data");
    }
    
    Element render() {
        return Group {
            {{"border-color", "#ff0"},
             {"border-size",   1.0}}, {
                Edit {{
                    {"bind",         "edit-data"},
                    {"region",       Region {R{32}, T{1}, R{1}, H{1}}},
                    {"border-color", "#ff0"},
                    {"border-size",  1.0}
                }},
                Label {{
                    {"text-label",   "filter:"},
                    {"text-align-x", Align::End},
                    {"region",       Region {R{42}, T{1}, W{7}, H{1}}}
                }},
                List {{
                    {"bind",         "resources"},
                    {"border-color", "#ff0"},
                    {"border-size",  1.0},
                    {"column-ids",   vec<List::Column> {
                        {"name", 0.66},
                        {"size", 32, Align::End}
                    }},
                    {"border-color", rgba {"#ff0"}},
                    {"region",       Region {L{1}, T{3}, R{1}, B{1}}}
                }}/*,
                Label {{
                    {"value",        "Bottom"},
                    {"region",       Region {L{1}, B{1}, W{6}, H{1}}}
                }}*/
            }
        };
    }
};

Args defaults = {
    {"window-width",  int(600) },
    {"window-height", int(1000)}
};

/// todo: A react-ive [norton] commander-style view.
int main(int c, const char *v[]) {
    return Window(c, v, defaults)([&] (Args &args) {
        return Commander(args);
    });
}
