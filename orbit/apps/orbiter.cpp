#include <orbit/sync.hpp>
#include <dx/type.hpp>
#include <dx/watch.hpp>


/// software creation for different platforms was meant to be abstracted away.
/// Orbiter is a reasonable shot at it.
///
int main(int argc, const char *argv[]) {
    Map     def          = Map {{ "project", "" }};
    Map     args         = Map(argc, argv, def);
    string  proj         = string(args["project"]);
    URI     uri          = "https://localhost:2200/";
    int64_t timeout_ms   = 60000;
    Path    project_path = args["project"] ? Path(args["project"]) : ".";
    str     s_pkg        = str::read_file(project_path / "package.json");
    var     pkg          = var::parse_json(s_pkg);
    Socket  sc           = Socket::connect(uri);
    auto   &depends      = pkg["depends"].map();

    ///
    if (sc) {
        sc.set_timeout(timeout_ms);
        for (auto &[name, dep]: depends) {
            URI   url = str(dep["url"]);
            str s_url = url; /// use the 'compact' mode instead of stringable flag?
        }
        ///
        console.log("request: {0}", { str(uri) });
        assert(pkg);
        
        Map dep = {};
        Station::send(sc, Station::Dependency, dep);

        /// spawn file watch
        auto w = Watch::spawn({ project_path }, {}, { Path::Recursion, Path::UseGitIgnores, Path::NoHidden },
                    [&](bool init, array<PathOp> &paths) {
            bool send_sync = false;
            for (auto &op: paths)
                if (op == PathOp::None || op == PathOp::Created || op == PathOp::Modified) {
                    Station::send(sc, Station::Resource, op.path);
                    send_sync = true;
                    console.log("resource: {0}", { op.path });
                }
            Station::send(sc, Station::Sync, null);
        });
        
        /// message loop
        for (;;) {
            auto  rcv = Station::recv(sc);
            auto   im = int(rcv);
            auto  &cc = rcv.content;
            ///
            if (!rcv)
                break;
            ///
            switch (im) {
                case Station::Pong: {
                    console.log("ping? pong!");
                    break;
                }
                
                case Station::Delivery: {
                    str name     = cc["name"];     /// name
                    str version  = cc["version"];  /// major.minor.patch (no build)
                    str platform = cc["platform"]; /// mac, win, lnx, ios, andr, web [ in that order. ]
                    str arch     = cc["arch"];     /// x64, arm64, arm32
                    str type     = cc["type"];     /// debug or release
                    var &bin     = cc["bin"];      /// binary image
                    /// -------------------------- ///
                    str res_name = var::format("products/{0}-{1}-{2}-{3}", { name, platform, arch, version });
                    bin.write_binary(res_name);    ///
                    break;
                }
                
                case Station::Activity: {
                    str text = cc["text"];
                    console.log("activity: {0}", {text});
                    break;
                }
                
                default:
                    break;
            }
        }
        /// broken out of message loop, stop watching
        w.stop();
    }
    ///
    return Async::await();
}
