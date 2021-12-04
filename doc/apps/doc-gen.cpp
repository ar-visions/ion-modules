#include <dx/dx.hpp>

auto default_args = Args {{"path", null}};

int main(int argc, const char *argv[]) {
    Args args = var::args(argc, argv, default_args);
    /// iterate through module folders relative to this cwd, unless a path is given
    auto  cwd = std::filesystem::current_path();
    str  path = args["path"] ? str(args["path"]) : str(cwd);
    console.log("cwd = {0}", {cwd});
    int  test = 1;
    return 0;
}
