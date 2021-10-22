
#include <ai/ai.hpp>
#include <ai/gen.hpp>
#include <media/image.hpp>

/// load ai model, load image, run inference
int main(int argc, const char* argv[])
{
    auto  def = Args {{"root",  "data"},
                      {"model", "avg"},
                      {"ds",    "cows,pigs"}};
    auto args = var::args(argc, argv, def);
    auto root = path_t(args["root"]);
    auto   ds = Dataset::parse(root, args["ds"]);
    auto  gen = Generator {args, ds};
    auto   dd = var("abc");
    
    dd["meta"] = true;
    
    // if json is not given, it doesnt require annotations
    gen({"avg"}, vec<str> {".json", ".jpg", ".png"},
        [&](path_t media, var &annots) -> Truths {
        // {"inputs":[{"image0.u8":[24,32,1]}],"output":1,"validation":["eval","fatigue","lookdown"]}
        // {"inputs":{"image0.u8":[24,32,1]} ... order of course matters.
        // starting with Image
        // the thrust of the api thus far has been very basic i/o, and that is fine but we need to fill in the abstract to Image var conversion
        // var now needs a shape!  That makes it more 1:1 with python number lib numpy.
        return null;
    });
    
    /*
    auto def  = Args {{"image", "str"}};
    auto args = var::args(argc, argv, def);
    ///
    if (args.size() == 0) {
        fprintf(stderr, "minimal <tflite model>\n");
        return 1;
    }
    auto   im = Image { args["image"], Image::Format::Rgba };
    auto   ai = AI    { args["model"] };
    auto   in = ai({ im });
    ///
    /// check if there is a memory leak.  may as well write a generalized test
    /// module for the images; test should have its own resource called res-test
    /// res, res-test
    ///
    */
    
    return 0;
}
