#pragma once
#include <dx/type.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

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
