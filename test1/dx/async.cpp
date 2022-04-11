import dx:type;

import <thread>
import <mutex>
import <condition_variable>

export module dx:async;
export {

struct Future;

typedef std::condition_variable ConditionV;

struct Process {
protected:
    static bool             init;
    static void             manager();
public:
    static ConditionV       cv_cleanup;
    static std::thread      th_manager;
    static std::mutex       mx_global;
    static std::mutex       mx_list;
    static array<Process *>   processes;
    static int              exit_code;
    bool                               deletable = false;
    std::mutex                         mx_self;
    std::function<void(var &)>         on_complete;
    std::function<void(var &)>         on_failure;
    std::function<var(Process *, int i)> fn;
    var                                data;
    std::vector<std::thread>           threads;
    int                                completed = 0;
    bool                               failure   = false;
    bool                               join      = false;
    Process();
   ~Process();
};

typedef std::function<var(Process *, int)> FnProcess;
typedef std::function<void(var &d)> FnFuture;

struct async {
    Process*        process;
    var             data;
    static void     thread(Process *, int i);
    static int      await();
    var&            sync();
    async          ();
    async          (str exec);
    async          (int c, FnProcess f);
    async          (FnProcess f);
    async          (const async &r);
    async&          operator=(const async &r);
    Future          then(FnFuture fn);
   ~async();
    operator Future();
    std::mutex      mx;
};

struct Customer {
    FnFuture fn;
    Customer(FnFuture fn):fn(fn) { }
};
struct CompleterData;
struct Completer;
struct Future   {
protected:
    CompleterData*  cdata;
public:
    Future&         then  (FnFuture fn);
    Future&         except(FnFuture fn);
    Future         (const Future &ref);
    Future         (Completer &c);
   ~Future         ();
    Future&         operator=(const Future &ref);
    void            sync();
};

struct CompleterData {
    var            data;
    std::mutex      mx;
    bool            completed;
    array<Customer>   l_success;
    array<Customer>   l_failure;
};

struct Completer {
    CompleterData*  cdata;
    Completer(std::function<void(var &)> &fn_success, std::function<void(var &)> &fn_failure);
    virtual        ~Completer() { }
    operator        Future();
};












































bool             Process::init;
std::thread      Process::th_manager;
ConditionV       Process::cv_cleanup;
std::mutex       Process::mx_global;
std::mutex       Process::mx_list;
array<Process *> Process::processes;
int              Process::exit_code = 0;

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

void async::thread(Process *p, int i) {
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

var &async::sync() {
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

int async::await() {
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

async::async(str exec) : async(1, [&](Process *p, int i) -> var {
    return int(std::system(exec.cstr()));
}) { }

async::async() { }
async::async(int count, FnProcess fn) {
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
            process->threads.emplace_back(std::thread(async::thread, process, i));
        Process::processes += process;
    }
}
async::async(std::function<var(Process *p, int i)> fn) : async(1, fn) { }

async::operator Future() {
    std::function<void(var &)> s, f;
    Completer c = { s, f };
    assert( process);
    assert(!process->on_complete);
    process->on_complete = [&, s, f](var &d) { s(d); };
    process->on_failure  = [&, s, f](var &d) { f(d); };
    return Future(c);
}

async::async(const async &r) {
    assert(false);
    process = r.process;
}

async &async::operator=(const async &r) {
    process = r.process;
    data    = r.data;
    return *this;
}

async::~async() {
    if (process && process->deletable)
        delete process; /// data needs to stick around until this async object is deleted
}

/// failing at the moment, not a graceful shutdown
void Future::sync() {
    std::mutex mx;
    /// requires indirection by CompleterData
    if (cdata) {
        cdata->mx.lock();
        if (!cdata->completed) {
            mx.lock();
            FnFuture fn = [mx=&mx](var &){ mx->unlock(); };
            cdata->l_success += fn;
            cdata->l_failure += fn;
        }
        cdata->mx.unlock();
    }
    mx.lock();
};

Future& Future::then(FnFuture fn) {
    cdata->mx.lock();
    cdata->l_success += Customer { fn };
    cdata->mx.unlock();
    return *this;
}

Future& Future::except(FnFuture fn) {
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
///
}
