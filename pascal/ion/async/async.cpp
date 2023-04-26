#include <async/async.hpp>

async::async() { }

///
async::async(size_t count, FnProcess fn) : async() {
    ///
    std::unique_lock<std::mutex> lock(process::mtx_list);

    /// create empty results [dynamic] vector [we need to couple the count into process, or perhaps bring it into async]
    d.proc      = process { count, fn };
    runtime &ps = d.proc.state;
    ps.handle   = d.proc.mem->grab();

    /// measure d.proc.rt address here
    if (count <= 0) {
        lock.unlock();
        process::procs.push(&ps);
        fn(ps, 0);
        process::procs.pop();
        lock.lock();
        d.proc = null; /// delete process here (it is out of the list by the lock time and thus should immediately delete)
        /// i think the above line is optional, as when the async falls out of scope, the ref-- delete results in trivial destruction which ref--'s
    } else {
        ///
        ps.threads = new std::vector<std::thread>();
        ps.threads->reserve(count);
        for (size_t i = 0; i < count; i++)
            ps.threads->emplace_back(std::thread(process::run, &ps, int(i)));
        ///
        process::procs.push(&ps);
    }
}

/// path to execute, we dont host a bunch of environment vars. ar.
async::async(path exec) : async(1, [&](runtime &proc, int i) -> mx {
    return int(std::system(exec.cs()));
}) { }
    
async::async(lambda<mx(runtime &, int)> fn) : async(1, fn) {}

array<mx> &async::sync() {
    /// wait for join to complete, set results internal and return
    for (;;) {
        d.mtx.lock();
        if (d.proc.joining()) {
            d.mtx.unlock();
            break;
        }
        d.mtx.unlock();
        usleep(10);
    }
    d.results = d.proc.state.results;
    return d.results;
}

/// await all async processes to complete
int async::await() {
    for (;;) {
        process::mtx_global.lock();
        if (!process::procs.len()) {
                process::mtx_global.unlock();
                break;
        }
        process::mtx_global.unlock();
        usleep(1000);
    }
    process::th_manager.join();
    return process::exit_code;
}

/// return future for this async
async::operator future() {
    lambda<void(mx)>   s, f;
    completer    c = { s, f };
    assert( d.proc);
    assert(!d.proc.state.on_done);
    d.proc.state.on_done     = [s, f](mx v) { s(v); };
    d.proc.state.on_failure  = [s, f](mx v) { f(v); };
    return future(c);
}