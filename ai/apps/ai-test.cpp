#include <ai/ai.hpp>
#include <ai/gen.hpp>
#include <media/image.hpp>


int main(int argc, cchar_t* argv[]) {
    auto def  = Map {{"image", "str"}};
    auto args = var::args(argc, argv, def);
    ///
    if (args.size() == 0)
        std::cerr << "ai <tflite model>\n";
    ///
    auto   im = Image { args["image"], Image::Format::Rgba };
    auto   ai = AI    { args["model"] };
    auto   in = ai({ im });
    return 0;
}
