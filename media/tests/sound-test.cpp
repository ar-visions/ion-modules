#include <media/audio.hpp>
#include <media/image.hpp>

int main() {
    Sound snd = {path_t { "engineering.mp3"  }};
    var   ann = {path_t { "engineering.json" }, var::Json};
    size_t  i = 0;
    assert(snd);
    assert(ann["audio"][i]["from"] == int(0));
    assert(ann["audio"][i]["to"]   == int(10));
    assert(ann["audio"][i]["values"]["value1"] == double(1.1));
    assert(ann["audio"][i]["values"]["value2"] == double(0.5));
    i++;
    assert(ann["audio"][i]["from"] == int(100));
    assert(ann["audio"][i]["to"]   == int(102));
    assert(ann["audio"][i]["values"]["value1"] == true);
    return 0;
}
