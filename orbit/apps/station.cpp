#include <orbit/sync.hpp>

auto default_args = Map  {{"sessions", null}, {"ion", null}};

///
int main(int argc, const char **argv) {
    Map        args = Map(argc, argv, default_args);
    URI         uri = "https://0.0.0.0:2200/";
    var  v_sessions = args.count("sessions") ? args["sessions"] : null;
    var       v_ion = args.count("ion")      ? args["ion"]      : null;
    Path   sessions = v_sessions;
    Path        ion = Path(v_ion);
    str         CML = "CMakeLists.txt";
    
    /// make symlink of cmake dir of the server root, which holds session folders
    assert(sessions.exists());
    assert(     ion.exists());
    ///
    ion.link(sessions / "ion-modules");
    
    /// to use: run orbiter, connect to station port 2200.
    Socket::listen(uri, [CML=CML, ion=ion, sessions=sessions](Socket sc) {
        Temp      session = Temp(sessions);
        Path  cmake_index = ion  / CML;
        cmake_index.link(session / CML);
        
        ///
        bool           keep = true;
      //auto     last_heard = ticks(); /// it will check each one for changes on orbit, though.  just as a rule
      //const   int timeout = 5 * 60000;
        str         project = null;
        str         version = null;
        array<str>   active = { };
        array<str>  garbage = { " ", "..", "/", "\\" };
        auto        is_sane = [&](str &v, bool nullable) {
            return (!v && nullable) || (v && !v.contains(garbage));
        };

        /// logging interface (used by console)
        auto activity = Logger<Always>([&](str &text) {
            Map content = {{"text", text}};
            Message("activity", content).write(sc);
        });

        /// set project name and version, if sane.
        auto  project_msg = [&](str name, str ver) -> bool {
            if (!is_sane(name, false) || !is_sane(ver, false))
                return false;
            project = name;
            version = ver;
            return true;
        };

        /// todo: deprecate direct use of path_t in favor of its wrapper
        auto repo_path = [](Path &root, str &project) -> Path {
            path_t s = var::format("{0}/checkout/{1}", { root, project });
            return s;
        };

        /// lets get the session str set.
        auto add_dependency = [&](Path &r_path, str &project, str &module, str &version) -> bool {
            Path d_path = session / "extern" / project;
            session    += SymLink { project, r_path };
            return true;
        };

        /// clone, checkout and symlink repo@version (goto dir if exists, else clone)
        /// if branch is truthy, you checkout branch
        auto dep_msg = [&](str project, str module, str version, str uri, str branch) -> bool {
            if (!project || !module || !uri)
                return false;
            Path      r_path = repo_path((Path &)sessions, project);
            bool    has_repo = r_path.dir();
            int         code = 0;
            if (!has_repo) {
                activity.log("cloning {2} : {1}, branch: {0}", { branch, uri, project });
                if (branch)
                    branch = var::format("--branch {0} ", {branch});
                auto  git = Async { var::format("git clone --depth 1 {0}{1} {2}", { branch, uri, project })};
                code      = git.sync();
                activity.log("exit-code: {0}", { code });
            }
            return (code == 0) ? add_dependency(r_path, project, module, version) : false;
        };

        /// 
        auto products_msg = [&](array<str> products) -> bool {
            /// this erases and replaces products
            /// list products to auto-build; mind you these are just looked at per auto-build increment
            return true;
        };

        ///
        auto activate_msg = [&](array<str> products) -> bool {
            /// assert they all exist as products.
            return true;
        };

        ///
        auto deactivate_msg = [&](array<str> products) -> bool {
            /// assert they all exist as products.
            return true;
        };

        ///
        auto ping_msg  = [&]() -> bool { return Station::send(sc, Station::Pong, null); };

        ///
        auto build_msg = [&]() -> bool {
            return true;
        };

        ///
        auto clean_msg = [&]() -> bool {
            return true;
        };

        ///
        auto resource_msg = [&](str name, var &data) -> bool {
            Path  path = session / name;
            console.log("write file: {0}, data size: {1}", { path, data.size() });
            return true;
        };

        ///
        auto delete_msg = [&](str name) -> bool {
            return Station::send(sc, Station::Delete, null);
        };
        
        auto sync_msg   = [&]() -> bool {
            /// start build, stop one if there is one going.
            return true;
        };

        ///
        while (keep) {
            auto      msg = Message(sc.uri, sc);
            auto     imsg = int(msg);
            auto      &cc = msg.content;
            bool     keep = true;
            ///
            std::string n = "name",   v = "version",  p = "project",
                        m = "module", d = "products", b = "branch",  u = "uri";
            ///
            switch (imsg) {
                case Station::Dependency: keep =        dep_msg(cc[n], cc[m], cc[v], cc[u], cc[b]); break;
                case Station::Project:    keep =    project_msg(cc[n], cc[v]); break;
                case Station::Resource:   keep =   resource_msg(cc[n], cc[u]); break;
                case Station::Delete:     keep =     delete_msg(cc[n]);        break;
                case Station::Products:   keep =   products_msg(cc[d]);        break;
                case Station::Activate:   keep =   activate_msg(cc[d]);        break;
                case Station::Deactivate: keep = deactivate_msg(cc[d]);        break;
                case Station::Sync:       keep =       sync_msg();             break;
                case Station::Ping:       keep =       ping_msg();             break;
                case Station::ForceBuild: keep =      build_msg();             break;
                case Station::ForceClean: keep =      clean_msg();             break;
                default:                  keep =            false;             break;
            }
        }
        ///
        return true;
    });
 
    return Async::await();
}
