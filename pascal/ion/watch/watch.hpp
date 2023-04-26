/// who throws a way a perfectly good Watch?!
#pragma once
#include <async/async.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

///
struct path_op:mx {
    struct members {
        path     p;
        size_t   path_index;
        path::op op;
    } &data;
    
    ctr(path_op, mx, members, data);
    inline bool operator==(path::op op) { return op == data.op; }
};

struct path_state:mx {
    struct state {
        path        res;
        size_t      path_index;
        i64         modified;
        bool        found;
    } &s;

    ctr(path_state, mx, state, s);
    inline void reset() { s.found = false; }
};

struct watch:mx {
    using fn = lambda<void(bool, array<path_op> &)>;

    struct state {
        bool             safe;
        bool        canceling;
        const int     polling = 1000;
        array<path>     paths;
        states<path::option> options;
        fn           watch_fn;
        array<path_op>    ops;
        int              iter;
        int           largest;
        array<str>       exts;
        map<path_state> path_states;
    } &s;

    ctr(watch, mx, state, s);

    watch(array<path> paths, array<str> exts, states<path::option> options, watch::fn watch_fn);
    
    void stop();
};
