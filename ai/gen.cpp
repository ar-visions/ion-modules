#include <ai/gen.hpp>
#include <data/async.hpp>



void Generator::operator()(str model, vec<str> exts, std::function<Truths(path_t, var &)> fn) {
    auto           processed = map<path_t, bool>();
    vec<DataW>     annots;
    std::mutex     mx;
    bool           init = true;
    
    for (Dataset &d: ds) {
        auto   dir = std::filesystem::directory_iterator(d.path);
        for (auto &e: dir) {
            auto   p = e.path();
            auto ext = p.extension();
            auto  ie = exts.index_of(ext);
            if (ie <= 0)
                continue;
            
            mx.lock();
            auto rp      = p.root_directory();
            auto js_file = path_t(str(rp) + str("/") + str(p.stem().string()) + str {".json"});
            auto js_req  = exts.index_of(".json") >= 0;
            if (processed.count(js_file) || (js_req && !std::filesystem::exists(js_file))) {
                mx.unlock();
                continue;
            }
            processed[js_file] = true;
            mx.unlock();
            
            auto contents = str(js_file);
            annots += { p, var::read_json(contents), d.w };
        }
    }
    double split = args.count("split") ? double(args["split"]) : 0.10;
    /// where annot_index % w.index == 0
    std::ofstream *olabels;
    vec<std::ofstream *> odata;
    Truth first_valid;
    auto async = Async(16, [&, split=split](Process *process, int index) -> var {
        int i = 0;
        bool init_check = true;
        for (auto &a: annots) {
            if ((i++ % index) != 0)
                continue;
            // its important to trace it to an exact step, but this would take a seed for a rand context instance, called by the lambda
            bool train = Rand::uniform(0.0, 1.0) >= split;
            auto     r = train ? a.w : 1.0;
            for (int i = 0; i < r; i++) {
                Truths truths = fn(a.p, a.data);
                for (auto &t: truths) {
                    if (!t)
                        continue;
                    if (init_check) {
                        mx.lock();
                        if (init) {
                            init_check = false;
                            init = false;
                            first_valid = t;
                            std::ofstream o_index(str::format("gen/{0}/index.json", {model}).cstr());
                            var          d_index = var(var::Map);
                            // inputs
                            str s_shape = "";
                            for (size_t i = 0; i < t.data.size(); i++) {
                                if (s_shape)
                                    s_shape += str {","};
                                var &d  = t.data[i]; // store the shape on pixels in image?
                                str key  = str::format("data{0}.{1}", {
                                    i, (t.data[i].c == var::ui8 ? "u8" : "f32")});
                                var dm  = var(var::Map);
                                dm[key]  = vec<int>::import(d.shape());
                                s_shape += str(dm);
                            }
                            std::string   s_index = std::string(d_index);
                            o_index.write(s_index.c_str(), s_index.length());
                            
                            olabels = new std::ofstream(
                                str::format("gen/{0}/labels.f32", {model}).cstr(),
                                std::ios::out | std::ios::binary);
                            for (size_t i = 0; i < t.data.size(); i++) {
                                assert(t.data[i].t == var::Array);
                                assert(t.data[i].c == var::ui8 || t.data[i].c == var::f32);
                                odata += new std::ofstream(
                                    str::format("gen/{0}/data{1}.{2}", {
                                        model, i, (t.data[i].c == var::ui8 ? "u8" : "f32")
                                    }).cstr(),
                                    std::ios::out | std::ios::binary);
                            }
                        }
                        mx.unlock();
                        init_check = false;
                    }
                    mx.lock();
                    // check data size consistency and output
                    assert(t.label.size() == first_valid.label.size());
                    assert(t.data.size()  == first_valid.data.size());
                    size_t sz = t.data.size();
                    for (size_t i = 0; i < sz; i++) {
                        var   &d = t.data[i];
                        var   &c = first_valid.data[i];
                        assert(var::type_check(d, c));
                        assert(d.size() == c.size());
                        if (d.c == var::ui8)
                            odata[i]->write((const char *)d.data<uint8_t>(), d.size() * sizeof(uint8_t));
                        else
                            odata[i]->write((const char *)d.data<float>(),   d.size() * sizeof(float));
                    }
                    // output labels
                    olabels->write((const char *)t.label.data(), t.label.size() * sizeof(float));
                    mx.unlock();
                }
            }
        }
        return null;
    });
    async.result();
}

