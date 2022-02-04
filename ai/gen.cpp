#include <ai/gen.hpp>
#include <dx/async.hpp>
#include <media/image.hpp>

void first_data(str model, Truth &schema, array<std::ofstream *> &odata, std::ofstream *& olabels) {
    std::ofstream o_index(var::format("gen/{0}/index.json", {model}).cstr());
    var d_index  = var(Type::Map);
    str s_shape  = "";
    for (size_t i = 0; i < schema.data.size(); i++) {
        if (s_shape)
            s_shape += str {","};
        var &d   = schema.data[i]; // store the shape on pixels in image?
        str key  = var::format("data{0}.{1}", {
            i, (schema.data[i].c == Type::ui8 ? "u8" : "f32")});
        var dmap   = var(Type::Map);
        dmap[key]  = array<int>(d.shape());
        s_shape += str(dmap);
    }
    string s_index = string(d_index);
    o_index.write(s_index.cstr(), s_index.size());
    olabels = new std::ofstream(
        var::format("gen/{0}/labels.f32", {model}).cstr(),
        std::ios::out | std::ios::binary);
    for (size_t i = 0; i < schema.data.size(); i++) {
        assert(schema.data[i].t == Type::Array);
        assert(schema.data[i].c == Type::ui8 || schema.data[i].c == Type::f32);
        odata += new std::ofstream(
            var::format("gen/{0}/data{1}.{2}", {
                model, i, (schema.data[i].c == Type::ui8 ? "u8" : "f32")
            }).cstr(),
            std::ios::out | std::ios::binary);
    }
}

void index_data(array<Dataset> &ds, array<str> &require, array<DataW> &index) {
    auto processed = map<path_t, bool>();
    for (Dataset &d: ds) {
        auto   dir = std::filesystem::directory_iterator(d.path);
        for (auto &e: dir) {
            auto   p = e.path();
            auto  id = str { d.path.string() } + str {"/"} + str { p.stem().string() };
            ///
            if (processed.count(id))
                continue;
            processed[id] = true;
            
            auto ex = [](path_t p) {
            ///
            auto  rp = p.string().substr(0, p.filename().string().length());
            auto  js = path_t(str(rp) + str(p.stem().string()) + str {".json"});
            str  vid = std::filesystem::exists(js) ? ".mp4"  : "";
            str  img = std::filesystem::exists(id + ".jpg")  ? (id + ".jpg")  :
                       std::filesystem::exists(id + ".jpeg") ? (id + ".jpeg") :
                       std::filesystem::exists(id + ".png")  ? (id + ".png")  : "";
            str  aud = std::filesystem::exists(id + ".mp4")  ? (id + ".mp4")  :
                       std::filesystem::exists(id + ".mp3")  ? (id + ".mp3")  : "";
            
            if (require.size() and !std::filesystem::exists(js))
                continue;
            ///
            /// image and audio files get their best path to resource selection here
            /// video and audio will not conflict
            var data = var(Type::Map);
            if (img) data[".image"] = img;
            if (aud) data[".audio"] = aud;
            data["annots"] = var { path_t { id + ".json" }, var::Json };
            ///
            /// continue if the required data does not exist; i believe .extension can count for files
            bool cont = false;
            for (str &r: require) {
                if (r[0] != '.' and data["annots"].count(r) == 0) {
                    cont = true;
                    break;
                }
                /// filter by resource availability
                if (r[0] == '.') {
                    auto res = path_t(str(rp) + str(p.stem().string()) + r);
                    if (!exists(res)) {
                        cont = true;
                        break;
                    }
                }
            }
            if (!cont)
                index += { p, data, d.w };
        }
    }
    index.shuffle();
}

void Gen(
        Map            &args,
        array<str>      require,
        array<Dataset> &ds,
        str             model,
        std::function<Truths(var &)> fn) {
    array<str>   idents;
    array<DataW> annots;
    bool         init = true;
    
    index_data(ds, require, annots);
    
    double split = args.count("split") ? double(args["split"]) : 0.10;
    /// where annot_index % w.index == 0
    std::ofstream       *olabels;
    array<std::ofstream *> odata;
    Truth                schema;
    std::mutex           mx;
    auto async = Async(16, [&, split=split](Process *process, int index) -> var {
        int  annot_index = 0;
        bool  init_check = true;
        for (auto &a: annots) {
            if ((annot_index++ % process->threads.size() != index))
                continue;
            // its important to trace it to an exact step, but this would take a seed for a rand context instance, called by the lambda
            bool train = Rand::uniform(0.0, 1.0) >= split;
            auto     r = train ? a.w : 1.0;
            for (int i = 0; i < r; i++) {
                Truths truths = fn(a.data);
                for (auto &t: truths) {
                    if (!t)
                        continue;
                    if (init_check) {
                        mx.lock();
                        if (init) {
                            schema = t;
                            first_data(model, schema, odata, olabels);
                            init_check = false;
                            init = false;
                        }
                    } else
                        mx.lock();
                    // check data size consistency
                    assert(t.label.size() == schema.label.size());
                    assert(t.data.size()  == schema.data.size());
                    size_t  sz = t.data.size();
                    for (size_t i = 0; i < sz; i++) {
                        var &d = t.data[i];
                        var &c = schema.data[i];
                        assert(var::type_check(d, c));
                        assert(d.size() == c.size());
                        if (d.c == Type::ui8)
                            odata[i]->write((cchar_t *)d.data<uint8_t>(), d.size() * sizeof(uint8_t));
                        else
                            odata[i]->write((cchar_t *)d.data<float>(),   d.size() * sizeof(float));
                    }
                    // output labels
                    olabels->write((cchar_t *)t.label.data(), t.label.size() * sizeof(float));
                    mx.unlock();
                }
            }
        }
        return null;
    });
    async.sync();
}

Truths if_image(var &data, Image::Format format, std::function<Truths(Image &)> fn) {
    Truths res;
    if (data.count(".image")) {
        Image im = Image { path_t { data[".image"] }, format };
        if (im)
            res = fn(im);
    }
    return res;
}

Truths if_audio(var &data, std::function<Truths(Audio &)> fn) {
    Truths res;
    if (data.count(".audio")) {
        Audio au = path_t { data[".audio"] };
        if (au)
            res = fn(au);
    }
    return res;
}
