#include <ux/ux.hpp>

struct Commander:node {
    declare(Commander);
    
    struct Members {
        str path;
    } m;
    
    path_t path;
    
    void define() {
        Define <str> {this, "path", &props.path, "."};
    }
    
    void mount() {
        update_resources();
    }
    
    void update_resources() {
        /// populate resources
        path = path_t(props.path);
        size_t iter = 0;
        while (!std::filesystem::is_directory(path) || iter++ == 100)
            path = path.parent_path();
        data = var(Type::Map);
        data["resources"] = {Type::Array};
        auto dir = std::filesystem::directory_iterator(path);
        for (auto &e: dir) {
            auto   ep = e.path();
            size_t sz = std::filesystem::is_regular_file(ep) ?
                        std::filesystem::file_size(ep)       : 0;
            auto  res = Map {{
                {"name", ep.stem().string()},
                {"ext",  ep.extension().string()},
                {"size", sz}
            }};
            data["resources"] += res;
        }
        //data["resolve"] = (FnFilter) [&](var &d) {
        //    return str(d);
        //};
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
                    {"column-ids",   array<List::Column> {
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


int main(int c, cchar_t *v[]) {
    return Console<Commander>(c, v);
}
