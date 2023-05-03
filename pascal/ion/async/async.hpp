#pragma once

///
struct customer:mx {
    customer(lambda<void(mx)> fn = lambda<void(mx)>()) : mx(alloc(&fn)) { }
    customer(const customer  &c) : mx(c.mem->grab()) { }
};

/// 
struct completer:mx {
    struct cdata {
        mx              vdata;
        mutex           mtx;
        bool            completed;
        array<customer> l_success;
        array<customer> l_failure;
    } &cd;

    completer() : mx(alloc<cdata>()), cd(ref<cdata>()) { }
    completer(lambda<void(mx)>& fn_success,
              lambda<void(mx)>& fn_failure) : completer() {
        fn_success   = [&](mx d) {
            auto &cd = ref<completer::cdata>();
            cd.completed = true;
            for (customer &s: cd.l_success)
                s.ref<FnFuture>()(d);
        };
        fn_failure   = [&](mx d) {
            auto &cd = ref<completer::cdata>();
            cd.completed = true;
            for (customer &f: cd.l_failure)
                f.ref<FnFuture>()(d);
        };
    }
};

/// how do you create a future?  you give it completer memory
struct future:mx {
    completer::cdata &cd;
    future (memory*           mem) : mx(mem),         cd(ref<completer::cdata>()) { }
    future (const completer &comp) : mx(comp.grab()), cd(ref<completer::cdata>()) { }
    
    int sync() {
        cd.mtx.lock();
        mutex mtx;
        if (!cd.completed) {
            mtx.lock();
            FnFuture fn   = [mtx=&mtx] (mx) { mtx->unlock(); };
            cd.l_success += fn;
            cd.l_failure += fn;
        }
        cd.mtx.unlock();
        mtx.lock();
        return 0;
    };

    ///
    future& then(FnFuture fn) {
        cd.mtx.lock();
        cd.l_success += customer(fn);
        cd.mtx.unlock();
        return *this;
    }

    ///
    future& except(FnFuture fn) {
        cd.mtx.lock();
        cd.l_failure += customer(fn);
        cd.mtx.unlock();
        return *this;
    }
};

struct runtime;
typedef lambda<mx(runtime&, int)> FnProcess;

typedef std::condition_variable ConditionV;
struct runtime {
    memory                    *handle;   /// handle on rt memory
    mutex                      mtx_self; /// output the memory address of this mtx.
    lambda<void(array<mx>&)>   on_done;  /// not the same prototype as FnFuture, just as a slight distinguisher we dont need to do a needless non-conversion copy
    lambda<void(array<mx>&)>   on_failure;
    size_t                     count;
    FnProcess				   fn;
    array<mx>                  results;
    std::vector<std::thread>  *threads;       /// todo: unstd when it lifts off ground. experiment complete time to crash and blast in favor of new matrix.
    int                        done      = 0; /// 
    bool                       failure   = false;
    bool                       join      = false;
    ///
};

struct process:mx {
protected:
    inline static bool init;
    
public:
    inline static ConditionV       cv_cleanup;
    inline static std::thread      th_manager;
    inline static std::mutex       mtx_global;
    inline static std::mutex       mtx_list;
    inline static doubly<runtime*>   procs;
    inline static int              exit_code = 0;

    static inline void manager() {
        std::unique_lock<std::mutex> lock(mtx_list);
        for (bool quit = false; !quit;) {
            cv_cleanup.wait(lock);
            bool cycle    = false;
            do {
                cycle     = false;
                num index = 0;
                ///
                for (runtime *state: procs) {
                    state->mtx_self.lock();
                    auto &ps = *state;
                    if (ps.done == ps.threads->size()) {
                        ps.mtx_self.unlock();
                        lock.unlock();

                        /// join threads
                        for (auto &t: *(ps.threads))
                            t.join();
                        
                        /// manage process
                        ps.mtx_self.lock();
                        ps.join = true;

                        /// 
                        procs.remove(index); /// remove -1 should return the one on the end, if it exists; this is a bool result not some integer of index to treat as.
                        (procs.len() == 0) ?
                            (quit = true) : (cycle = true);
                        lock.lock();
                        ps.mtx_self.unlock();
                        break;
                    }
                    index++;
                    ps.mtx_self.unlock();
                }
            } while (cycle);
            /// dont unlock here because of the implicit behaviour of condition_variable
        }
        lock.unlock();
    }

    /// this is 1 time alloc per async and i want to isolate all of this to 1 lambda service
    runtime &state;

    static void run(runtime *rt, int w) {
        runtime &state = *rt;
        /// run (fn) the work (p) on this thread (i)
        mx r = state.fn(state, w);
        state.mtx_self.lock();

        state.failure   |= !r;
        state.results[w] =  r; /// set results at worker index w

        /// wait for completion of one (we coudl combine c check inside but not willing to stress test that atm)
        mtx_global.lock();
        bool im_last = (++state.done == state.count);
        mtx_global.unlock();

        /// if all complete, notify condition var after calling completer/failure methods
        if (im_last) {
            if (state.on_done)
                !state.failure ? state.on_done(state.results) : state.on_failure(state.results);

            mtx_global.lock();
            cv_cleanup.notify_all();
            mtx_global.unlock();
        }
        state.mtx_self.unlock();

        /// wait for job set to complete
        for (; state.done != state.count;)
            usleep(1);
    }

    ///
    ctr(process, mx, runtime, state);
    process(size_t count, FnProcess fn) : process(alloc<runtime>()) {
       if(!init) {
           init       = true;
           th_manager = std::thread(manager);
       }
       state.fn       = fn;
       state.count    = count;
       if (count)
            state.results  = array<mx> { count, count, [](num) -> mx { return mx(); } };
    }

    inline bool joining() const { return state.join; }
};

/// async is out of sync with the other objects.
struct async {
    ///
    struct delegation {
        process         proc;
        array<mx>       results;
        mutex           mtx; /// could be copy ctr
    } d;

    async();
    ///
    async(size_t count, FnProcess fn);

    /// path to execute, we dont host a bunch of environment vars. ar.
    async(path exec);
    
    async(lambda<mx(runtime &, int)> fn);

    array<mx> &sync();

    /// await all async processes to complete
    static int await();

    /// return future for this async
    operator future();
};

struct sync:async {
    sync(size_t count, FnProcess fn) : async(count, fn) { }
    sync(path p) : async(p) { }

    /// call array<S> src -> T conversion
    template <typename T>
    operator array<T>() { return     async::sync();     }
    operator      int() { return int(async::sync()[0]); }
};
