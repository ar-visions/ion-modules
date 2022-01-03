#pragma once
#include <dx/async.hpp>
// macOS doesnt support notifications [/kicks dirt]; get working on Linux and roll into Watch (resources top-level api)
//#include <sys/inotify.h>

struct PathOp {
    enum Op {
        None,
        Deleted,
        Modified,
        Created
    } type;
    path_t path;
    PathOp(path_t &path, Op type): type(type), path(path) { }
};

struct PathState {
    path_t      path;
    path_date_t modified;
    bool        found;
};

/// a protected-struct concept with Async service
struct Watch {
    typedef std::function<void(bool, vec<PathOp> &)> Fn;
protected:
    bool         safe = false;
    bool    canceling = false;
    const int polling = 1000;
    path_t       path;
    Fn          watch;
    vec<PathOp>   ops;
    int          iter = 0;
    int       largest = 0;
    std::vector<const char *> exts;
    std::unordered_map<path_t, PathState> pstate;
public:
    ///
    /// initialize, modified, created, deleted
    static Watch &spawn(path_t path, std::vector<const char *> exts,
                        std::function<void(bool, vec<PathOp> &)> watch) {
        ///
        Watch *pw = new Watch();
        pw->path  = path;
        pw->watch = watch;
        pw->exts  = exts;
        ///
        Async { [pw] (Process *p, int index) -> var {
            Watch &w = *(Watch *)pw;
            while (!w.canceling) {
                console.log("iterate..");
                w.ops = vec<PathOp>(32 + w.largest * 2);
                
                /// flag for deletion
                for (auto &[k,v]: w.pstate)
                    v.found = false;
                
                /// populate modified and created
                resources(w.path, w.exts, [&](path_t &p) {
                    auto ftime = std::filesystem::last_write_time(p);
                    if (w.pstate.count(p) == 0) {
                        PathOp::Op op = (w.iter == 0) ? PathOp::None : PathOp::Created;
                        w.pstate[p]   = { p, ftime, true };
                        w.ops        += { p, op };
                    } else {
                        PathState &ps = w.pstate[p];
                        ps.found = true; /// unflag
                        if (ps.modified != ftime) {
                            w.ops += { p, PathOp::Modified };
                            ps.modified = ftime;
                        }
                    }
                });
                
                /// populate deleted
                bool cont;
                do {
                    cont = false;
                    for (auto &[k,v]: w.pstate) {
                        if (!v.found) {
                            w.ops += { v.path, PathOp::Deleted };
                            w.pstate.erase(k);
                            cont = true;
                            break;
                        }
                    }
                } while (cont);
                
                /// notify user if ops
                if (w.ops.size())
                    w.watch(w.iter == 0, w.ops);
                
                /// track largest set for future reservation
                w.largest = std::max(w.largest, int(w.ops.size()));
                
                /// wait for polling period
                w.iter++;
                usleep(1000 * w.polling);
            }
            console.log("canceling watch.");
            w.safe = true;
            return null;
        }};
        return *pw;
    }
    
    void stop() {
        if (!safe) {
            canceling = true;
            while (!safe) { usleep(1); }
        }
        delete this;
    }
};
