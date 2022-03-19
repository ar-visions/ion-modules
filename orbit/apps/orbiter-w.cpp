#include <orbit/sync.hpp>
#include <dx/type.hpp>
#include <dx/watch.hpp>

/// Orbiter sync-client
/// 
int main(int argc, const char *argv[]) {
    Map     def          = Map {{ "project", "" }};
    Map     args         = Map(argc, argv, def);
    string  project      = string(args["project"]);
    URI     uri          = "https://localhost:2200/";
    int64_t timeout_ms   = 60000;
    Path    project_path = Path(args["project"] ? project.cstr() : ".");
    str     s_pkg        = str::read_file(project_path / "package.json");
    var     pkg          = var::parse_json(s_pkg);
    Socket  sc           = Socket::connect(uri);
    auto   &repos        = pkg["repos"].map();
    
    /// validate package.json before we proceed
    console.assertion(pkg, "package.json not found");
    
    /// lets sync with server before we build on server
    if (sc) {
        sc.set_timeout(timeout_ms);
        
        /// these are the roots kept in sync with the server
        array<Path> sync_roots = {};
        
        /// send project message, this says the project and version
        /// wheeeeere do we get the version from?  
        Station::send(sc, Station::Project, Map {{"project", pkg["name"]}});

        /// for each step, output what is going on at the server. and proceed on that path.
        for (auto &[project, dep]: repos) {
            str   version = dep.count("version") == 0 ? str(null) : str(dep["version"]);
            str      repo = dep["repo"];
            Map       msg = {
                {"project", project},
                {"repo",    repo},
                {"version", version}
            };
            
            /// sync mode
            if (sync) {
                /// reject if the dependency project folder does not exist
                Path local_path = project_path / ".." / module;
                Path   pkg_path =   local_path / "package.json";
                bool  different =  !local_path.same_as(project_path);
                
                /// rejection criteria
                console.assertion(local_path.exists(), "local path {0} does not exist",         { local_path });
                console.assertion(  pkg_path.exists(), "package.json not found: {0}",           { module     });
                console.assertion( different,          "project:{0} need not depend on itself", { project    });
                
                /// add path to sync roots
                sync_roots += local_path;
            } else {
                /// in the case of a versioned dependency,
                /// its product must exist in version form in the products dir once checked out
                /// these files need not exist on client, so nothing is done here.
            }
            
            /// send dependency
            Station::send(sc, Station::Dependency, dep);
        }
        
        /// require >= 1 sync_roots
        console.assertion(sync_roots, "no module roots to launch");
        std::mutex mx;
        
        /// spawn file watch for each sync root
        auto w = Watch::spawn(sync_roots, {}, { Path::Recursion, Path::UseGitIgnores, Path::NoHidden },
                    [&](bool init, array<PathOp> &paths) {
            
            /// this blocks in a useful way, just need a mutex for keeping things in sync i think.
            mx.lock();
            bool send_sync = false;
            for (auto &op: paths)
                if (op == PathOp::None || op == PathOp::Created || op == PathOp::Modified) {
                    var content = Map {
                        { "data",      var { op.path, var::Binary }},
                        { "rel_path",  op.path.relative(sync_roots[op.path_index])},
                        { "sync_root", int(op.path_index) }
                    };
                    /// this should work simply with:
                    Station::send(sc, Station::Resource, content);
                    send_sync = true;
                    console.log("resource: {0}", { op.path });
                }
            
            /// notify server that we're ok to go
            /// if it ever gets a resource at all, it will stop the build it has going, wait for sync until starting a new build
            Station::send(sc, Station::SyncComplete, null);
            mx.unlock();
        });
        
        /// message loop
        for (;;) {
            auto  rcv = Station::recv(sc);
            auto   im = int(rcv);
            auto  &cc = rcv.content;
            
            /// dont let watcher and message-loop disrupt each other
            mx.lock();
            ///
            if (!rcv)
                break;
            ///
            switch (im) {
                case Station::DeliveryComplete: { console.log("delivery: {0}", { cc["path"] }); break; }
                case Station::Activity:         { console.log("activity: {0}", { cc["text"] }); break; }
                case Station::Pong:             { console.log("pong");                          break; }
                case Station::DeliveryInbound:  {
                    Path product_path = var::format("products/{0}", { cc["path"] });
                    if (!product_path.write({ }))
                        console.fault("create failure on product: {0}", { product_path });
                    break;
                }
                ///
                case Station::DeliveryPacket: {
                    var         &data = cc["data"];
                    Path product_path = var::format("products/{0}", { cc["path"] });
                    ///
                    console.assertion(data, "invalid packet");
                    if (data) {
                        size_t     sz = data.size();
                        auto      arr = array<uint8_t> { data.data<uint8_t>(), sz };
                        bool  written = product_path.write(arr);
                        ///
                        console.assertion(written, "failed to write {0} bytes for path {1}", { sz, product_path });
                    }
                    break;
                }
                ///
                default:
                    break;
            }
            mx.unlock();
        }
        mx.unlock();
        
        /// broken out of message loop, stop watching
        w.stop();
    }
    ///
    return async::await();
}
