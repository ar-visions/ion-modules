import <iostream>
import <chrono>
import <iomanip>
import <fstream>
import <filesystem>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

import dx:nan;
import dx:array;
import dx:async;
import dx:var;

export module dx:watch;
export {

struct PathOp {
    enum Op {
        None,
        Deleted,
        Modified,
        Created
    } type;
    Path   path;
    size_t path_index; /// identity of element at index in path array given to Watch
    PathOp(Path &path, size_t path_index, Op type): type(type), path(path), path_index(path_index) { }
    bool operator==(Op t) { return t == type; }
};

struct PathState {
    Path        path;
    size_t      path_index;
    int64_t     modified;
    bool        found;
};

/// a protected-struct concept with async service
struct Watch {
    typedef std::function<void(bool, array<PathOp> &)> Fn;
protected:
    bool         safe = false;
    bool    canceling = false;
    const int polling = 1000;
    array<Path>   paths;
    Fn       watch_fn;
    array<PathOp>   ops;
    int          iter = 0;
    int       largest = 0;
    array<str>     exts;
    std::unordered_map<str, PathState> path_states;
public:
    /// initialize, modified, created, deleted
    static Watch &spawn(array<Path> paths, array<str> exts, FlagsOf<Path::Flags> flags,
                        std::function<void(bool, array<PathOp> &)> watch_fn) {
        ///
        Watch *pw = new Watch();
        pw->paths    = paths;
        pw->watch_fn = watch_fn;
        pw->exts     = exts;
        ///
        async { [pw, flags] (Process *p, int index) -> var {
            Watch &watch = *(Watch *)pw;
            while (!watch.canceling) {
                watch.ops = array<PathOp>(32 + watch.largest * 2);
                
                /// flag for deletion
                for (auto &[k,v]: watch.path_states)
                    v.found = false;
                
                /// populate modified and created
                size_t path_index = 0;
                for (Path path:watch.paths) {
                    path.resources(watch.exts, flags, [&](Path p) {
                        if (!p.exists())
                            return;
                        str       key = p;
                        int64_t mtime = p.modified_at();
                        ///
                        if (watch.path_states.count(key) == 0) {
                            PathOp::Op op    = (watch.iter == 0) ? PathOp::None : PathOp::Created;
                            watch.ops       += { p, path_index, op };
                            watch.path_states[key] = { p, path_index, mtime, true };
                        } else {
                            PathState &ps    = watch.path_states[key];
                            ps.found         = true;
                            if (ps.modified != mtime) {
                                watch.ops   += { p, path_index, PathOp::Modified };
                                ps.modified  = mtime;
                            }
                        }
                    });
                    path_index++;
                }
                
                /// populate deleted
                bool cont;
                do {
                    cont = false;
                    for (auto &[k,v]: watch.path_states) {
                        if (!v.found) {
                            watch.ops += { v.path, v.path_index, PathOp::Deleted };
                            watch.path_states.erase(k);
                            cont = true;
                            break;
                        }
                    }
                } while (cont);
                
                /// notify user if ops
                if (watch.ops.size())
                    watch.watch_fn(watch.iter == 0, watch.ops);
                
                /// track largest set for future reservation
                watch.largest = std::max(watch.largest, int(watch.ops.size()));
                
                /// wait for polling period
                watch.iter++;
                usleep(1000 * watch.polling);
            }
            
            ///
            console.log("canceling watch.");
            watch.safe = true;
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
///
}