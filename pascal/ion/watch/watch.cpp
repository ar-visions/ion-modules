#include <watch/watch.hpp>

watch::watch(array<path> paths, array<str> exts, states<path::option> options, watch::fn watch_fn) : watch() {
    s.paths       = paths;
    s.watch_fn    = watch_fn;
    s.exts        = exts;
    s.options     = options;
    state&     ss = s;
    ///
    async { size_t(1), [ss, options] (runtime &p, int index) -> var { /// give w a ref and copy into lambda
        state& s = (state &)ss;
        while (!s.canceling) {
            s.ops = array<path_op>(size_t(32 + s.largest * 2));
            
            /// flag for deletion
            for (field<path_state> &f: s.path_states) {
                path_state &ps = f.value;
                ps.s.found     = false;
            }
            
            /// populate modified and created
            size_t path_index = 0;
            for (path &res:s.paths) {
                res.resources(s.exts, options, [&](path p) {
                    if (!p.exists())
                        return;
                    str       key = p;
                    i64     mtime = p.modified_at();
                    ///
                    if (s.path_states.count(key) == 0) {
                        s.ops             +=    path_op::members { p, path_index, (s.iter == 0) ? path::op::none : path::op::created };
                        s.path_states[key] = path_state::state   { p, path_index, mtime, true };
                    } else {
                        auto &ps         = s.path_states[key].ref<path_state::state>();
                        ps.found         = true;
                        if (ps.modified != mtime) {
                            s.ops       += path_op::members { p, path_index, path::op::modified };
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
                /// k = path, v = path_state
                for (auto &[v,k]: s.path_states) {
                    if (!v.s.found) {
                        s.ops += path_op::members { v.s.res, v.s.path_index, path::op::deleted };
                        s.path_states.remove(k);
                        cont = true;
                        break;
                    }
                }
            } while (cont);
            
            /// notify user if ops
            size_t sz = s.ops.len();
            if (sz)
                s.watch_fn(s.iter == 0, s.ops);
            
            /// track largest set for future reservation
            s.largest = math::max(s.largest, int(sz));
            
            /// wait for polling period
            s.iter++;
            usleep(1000 * s.polling);
        }
        
        ///
        s.safe = true;
        return null;
    }};
}

void watch::stop() {
    if (!s.safe) {
        s.canceling = true;
        while (!s.safe) { usleep(1); }
    }
}