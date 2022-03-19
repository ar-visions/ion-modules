#include <orbit/sync.hpp>

auto default_args = Map  {{"sessions", null}, {"ion", null}};

/*  /// this will assist in header production as well it just needs to be in the module files is all
    /// you essentially install headers, thats all.
    /// install(header), install(product), that is all. type derrived from there
    /// we like friendly syntax and succinct expression of intent.
 
    /// one option is to write the messaging protocol in the isolated componentized form.
    /// how would you
 
    /// all we have is a relative path from products
    str module   = cc["module"];   /// module
    str version  = cc["version"];  /// major.minor.patch (no build)
    str platform = cc["platform"]; /// mac, win, lnx, ios, andr, web [ in that order. ]
    str arch     = cc["arch"];     /// x64, arm64, arm32
    str type     = cc["type"];     /// debug or release
    var &bin     = cc["bin"];      /// binary image
    /// -------------------------- ///
    str res_name = var::format("products/{0}-{1}-{2}-{3}", { module, platform, arch, version });
    bin.write_binary(res_name);    ///
    break; */

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
        Temp        session = Temp(sessions);
        Path    cmake_index = ion  / CML;
        string this_project = null;
        string this_version = null;
        bool     auto_build = false;
        
        ///
        cmake_index.link(session / CML);
        
        ///
        auto is_sane = [&](str v, bool nullable) {
            static auto garbage = array<str> { " ", "..", "/", "\\" };
            return (!v && nullable) || (v && !v.contains(garbage));
        };

        /// logging interface (used by console)
        auto        activity = Logger<Always>([&](str &text) {
            Map      content = {{"text", text}};
            Message("activity", content).write(sc);
        });
        
        /// add resource
        auto  m_sync_inbound = [&](var &content) -> bool {
            string  rel_path = content["path"];
            string   project = content["project"];
            Path    abs_path = session / project / rel_path;
            return abs_path.remove_all();
        };
        
        /// append
        auto   m_sync_packet = [&](var &content) -> bool {
            var        &data = content["data"];
            string  rel_path = content["path"];
            string   project = content["project"];
            Path    abs_path = session / project / rel_path;
            return abs_path.append(data);
        };
        
        /// copy from tmp to src # not needed
        auto m_sync_complete = [&](var &content) -> bool {
            return true;
        };
        
        /// auto-build on/off
        auto     m_autobuild = [&](var &content) -> bool {
            auto_build = content["on"];
            return true;
        };
        
        ///
        auto        m_delete = [&](var &content) -> bool {
            string  rel_path = content["path"];
            string   project = content["project"];
            Path    abs_path = session / project / rel_path;
            console.assertion(abs_path.exists(), "Something strange.");
            return abs_path.remove_all();
        };
        
        ///
        auto          m_repo = [&](var &content) -> bool {
            string      repo = content["repo"];
            string   project = content["project"];
            string   version = content["version"];
            string    branch = content["branch"];
            
            /// params: repo, project
            Path      r_path = sessions / "checkout" / project; /// switch between git and rpath
            bool  local_repo = r_path.is_dir();
            int         code = 0;
            
            ///
            if (!local_repo) {
                /// assert that its an https.
                console.assertion(repo.starts_with("https://"), "remote repo requires https");
                
                ///
                activity.log("cloning {2} : {1}, branch: {0}", { branch, repo, project });
                if (branch) branch = var::format("--branch {0} ", {branch});
                auto  git = async { var::format("git clone --depth 1 {0}{1} {2}", { branch, repo, project })};
                code      = git.sync();
                activity.log("exit-code: {0}", { code });
            }
            
            ///
            if (code == 0) {
                Path d_path = session / "extern" / project;
                /// assert that we can create this?
                session    += SymLink { project, r_path };
                return true;
            }
            return false;
        };
        
        auto    m_ping = [&](var &content) -> bool {
            return false;
        };
        
        auto    m_build = [&](var &content) -> bool {
            return false;
        };
        
        auto    m_clean = [&](var &content) -> bool {
            return false;
        };

        /// project message (version
        auto  m_project = [&](var &content) -> bool {
            str project = content["project"];
            str version = content["version"];
            if (!is_sane(project, false) || !is_sane(version, false))
                return false;
            this_project = project;
            this_version = version;
            return true;
        };
        
        map<Station::Type, std::function<bool(var &)>> table = {
            { Station::Repo,         m_repo          },
            { Station::Project,      m_project       },
            { Station::SyncInbound,  m_sync_inbound  },
            { Station::SyncPacket,   m_sync_packet   },
            { Station::SyncComplete, m_sync_complete },
            { Station::Delete,       m_delete        },
            { Station::AutoBuild,    m_autobuild     },
            { Station::Ping,         m_ping          },
            { Station::ForceBuild,   m_build         },
            { Station::ForceClean,   m_clean         }
        };
        Station::Type type = Station::None;
        Message       msg;
        do {
             msg  = Message(sc.uri, sc);
             type = (Station::Type)int(msg);
        } while (table[type](msg.content));
        ///
        return true;
    });
 
    return async::await();
}
