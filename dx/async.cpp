#include <dx/var.hpp>

bool            Process::init;
std::thread     Process::th_manager;
ConditionV      Process::cv_cleanup;
std::mutex      Process::mx_global;
std::mutex      Process::mx_list;
vec<Process *>  Process::processes;
int             Process::exit_code = 0;

void Process::manager() {
    std::unique_lock<std::mutex> lock(mx_list);
    for (bool quit = false; !quit;) {
        cv_cleanup.wait(lock);
        bool cycle = false;
        do {
            cycle  = false;
            int index = 0;
            for (auto p: processes) {
                p->mx_self.lock();
                if (p->completed == p->threads.size()) {
                    p->mx_self.unlock();
                    lock.unlock();
                    for (auto &t: p->threads)
                        t.join();
                    p->mx_self.lock();
                    if (p->deletable)
                        delete p;
                    else
                        p->join = true;
                    processes -= size_t(index);
                    (processes.size() == 0) ?
                        (quit = true) : (cycle = true);
                    lock.lock();
                    p->mx_self.unlock();
                    break;
                }
                index++;
                p->mx_self.unlock();
            }
        } while (cycle);
        /// dont unlock here because of the implicit behaviour of condition_variable
    }
    lock.unlock();
}

Process::Process() {
    if (!init) {
         init = true;
         th_manager = std::thread(manager);
    }
}

Process::~Process() { }

void Async::thread(Process *p, int i) {
    p->mx_self.lock();
    size_t c = p->threads.size();
    p->mx_self.unlock();
    var    r = p->fn(p, i);
    p->mx_self.lock();
    if (r)
        p->data =  r;
    p->failure |= !r;
    Process::mx_global.lock();
    bool all_complete = false;
    if (++p->completed == c)
         all_complete = true;
    Process::mx_global.unlock();
    if (all_complete) {
        var &d = p->data;
        if (p->on_complete)
            p->failure ? p->on_failure(d) : p->on_complete(d);
        Process::mx_global.lock();
        Process::cv_cleanup.notify_all();
        Process::mx_global.unlock();
    }
    p->mx_self.unlock();
    for (;p->completed != c;)
        usleep(1);
}

var &Async::result() {
    for (;;) {
        mx.lock();
        if (process->join)
            break;
        mx.unlock();
        usleep(10);
    }
    data = process->data;
    delete process;
    process = null;
    return data;
}

int Async::await() {
    for (;;) {
        Process::mx_global.lock();
        if (Process::processes.size() == 0) {
            Process::mx_global.unlock();
            break;
        }
        Process::mx_global.unlock();
        usleep(1000);
    }
    Process::th_manager.join();
    return Process::exit_code;
}

Async::Async() { }
Async::Async(int count, std::function<var(Process *p, int i)> fn) {
    std::unique_lock<std::mutex>  lock(Process::mx_list);
    process = new Process;
    process->fn = fn;
    if (count <= 0) {
        Process::processes += process;
        lock.unlock();
        fn(process, 0);
        lock.lock();
        Process::processes -= Process::processes.size() - 1;
        delete process;
    } else {
        process->threads = std::vector<std::thread>();
        process->threads.reserve(count);
        for (size_t i = 0; i < count; i++)
            process->threads.emplace_back(std::thread(Async::thread, process, i));
        Process::processes += process;
    }
}
Async::Async(std::function<var(Process *p, int i)> fn) : Async(1, fn) { }

Async::operator Future() {
    std::function<void(var &)> s, f;
    Completer c = { s, f };
    assert( process);
    assert(!process->on_complete);
    process->on_complete = [&, s, f](var &d) { s(d); };
    process->on_failure  = [&, s, f](var &d) { f(d); };
    return Future(c);
}

Async::Async(const Async &r) {
    assert(false);
    process = r.process;
}

Async &Async::operator=(const Async &r) {
    process = r.process;
    data    = r.data;
    return *this;
}

Async::~Async() {
    if (process && process->deletable)
        delete process; /// data needs to stick around until this Async object is deleted
}

Future& Future::then(std::function<void(var &d)> fn) {
    cdata->mx.lock();
    cdata->l_success += Customer { fn };
    cdata->mx.unlock();
    return *this;
}

Future& Future::except(std::function<void(var &d)> fn) {
    cdata->mx.lock();
    cdata->l_failure += Customer { fn };
    cdata->mx.unlock();
    return *this;
}

Future:: Future(const Future &r) : cdata(r.cdata) { }
Future:: Future(Completer &c)    : cdata(c.cdata) { }
Future::~Future() { }

Future& Future::operator=(const Future &r) {
    cdata = r.cdata;
    return *this;
}

Completer::operator Future() {
    return Future(*this);
}

Completer::Completer(std::function<void(var &)> &fn_success, std::function<void(var &)> &fn_failure) {
    cdata = new CompleterData(); // not free'd by this of course; success/error operation deletes
    fn_success = [cdata=cdata](var& d) {
        cdata->completed = true;
        for (auto s: cdata->l_success)
            s.fn(d);
    };
    fn_failure = [cdata=cdata](var& d) {
        cdata->completed = true;
        for (auto f: cdata->l_failure)
            f.fn(d);
    };
}
