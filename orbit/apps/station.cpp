
#include <dx/dx.hpp>
#include <web/web.hpp>  // these resources auto-packaged kthx
#include <orbit/sync.hpp>

auto default_args = Args {{"root", null}};

struct Temp {
    path_t path;

    static path_t uid(path_t &base) {
        path_t path;
        do {
            path = str::fill(6, [](size_t) { return Rand::uniform('0', '9'); });
        } while (std::filesystem::exists(base / path));
        return path;
    }
    
    Temp(path_t b) {
        path_t base = b.length() ? b : "/tmp";
        path = uid(base);
        assert(path.length() > 0 && !std::filesystem::exists(path));
        std::filesystem::create_directory(path);
    }

    ~Temp() {
        if (path.length())
            std::filesystem::remove_all(path);
    }
};

int main(int argc, char[] argv) {
    auto   a = var::args(argc, argv);
    ///
    Async { [a=a] (Process *p, int index) -> var {
        return Socket::listen("https://0.0.0.0:2200", [aa=aa](Socket sc) {
            var         &args = (var &)a;
            Temp      session = path_t(args["root"]); /// session equals temp store
            bool         keep = true;          /// repos are fetched and stored just once
            auto   last_heard = ticks();       /// it will check each one for changes on orbit, though.  just as a rule
            const int timeout = 5 * 60000;     ///
            str       project = null;
            str       version = null;
            vec<str>   active = {};
            vec<str>  garbage = {" ", "..", "/", "\\"};
            auto      is_sane = [&](str &v, bool nullable) {
                return (!v && nullable) || (v && v.contains(garbage));
            };

            /// simple logging interface taking a console log wrapper
            auto activity = Logger([](str text) {
                auto m = Message(Activity, text);
                sc.write(m);
            });

            /// set project name and version, if sane.
            auto  project_msg = [&](str name, str ver) -> bool {
                if (!is_sane(name) || !is_sane(ver))
                    return false;
                project = name;
                version = v;
                return true;
            };

            /// clone, checkout and symlink repo@version (goto dir if exists, else clone)
            /// if branch is truthy, you checkout branch
            auto      dep_msg = [&](str project, str module, str version, str uri, str branch) -> bool {
                if (!project || !module || !uri)
                    return false;
                path_t repo_path = str::format("{0}/checkout/{1}", root, project);
                bool    has_repo = exists(repo_path);
                if (!has_repo) {
                    auto     git = Process { str::format("git clone --depth 1 --branch {0} {1} {2}", { branch, uri, project }) };
                    auto    code = git.sync();
                    if (code != 0) {
                        /// log this
                        activity.log(); /// use the same interface as 'console', except this sends data back over tls
                        return false;
                    }
                }
                /// > git clone --depth 1  --branch <branch> url
                /// exec shell git checkout uri checkout/project-version
                //if (!add_dependency(project, module, version, uri));
                return true;
            };

            /// 
            auto products_msg = [&](vec<str> products) -> bool {
                /// this erases and replaces products
                /// list products to auto-build; mind you these are just looked at per auto-build increment
            };

            ///
            auto activate_msg = [&](vec<str> products) -> bool {
                /// assert they all exist as products.
            };

            ///
            auto deactivate_msg = [&](vec<str> products) -> bool {
                /// assert they all exist as products.
            };

            ///
            while (keep) {
                auto      msg = Message(sc);       /// read from client (returns None on failure) (would like a binary mode here)
                auto     smsg = str(msg);
                auto       sm = StationMessage(smsg);
                auto     imsg = int(sm);
                str       arg = msg.content == Type::Str ? msg.content : null;
                auto    close = [&]() { keep_alive = false; };
                auto      &cc = msg.content;
                bool     keep = true;
                ///
                switch (imsg) {
                    case       Ping:                  msg.respond(Pong);                                    break;
                    case    Project: keep =       project_msg(cc["name"], cc["version"] ));                 break;
                    case Dependency: keep =           dep_msg(cc["project"], cc["module"], cc["version"]))  break;
                    case   Resource: keep =           res_msg(cc["project" ], cc["module"], cc["version"])) break;
                    case   Products: keep =      products_msg(cc["products"])) break;
                    case   Activate: keep =      activate_msg(cc["products"])) break;
                    case Deactivate: keep =    deactivate_msg(cc["products"])) break;
                    case ForceBuild: keep =         build_msg()) break;
                    case ForceClean: keep =         clean_msg()) break;
                    default:         keep = false;  break;
                }
            }
            return true;
        });
    };
    return Async::await();
}