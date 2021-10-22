#pragma once
#include <data/data.hpp>
#include <thread>
#include <mutex>

struct Future;

typedef std::condition_variable ConditionV;

struct Process {
protected:
    static bool           init;
    static void           manager();
public:
    static ConditionV     cv_cleanup;
    static std::thread th_manager;
    static std::mutex     mx_global;
    static vec<Process *> processes;
    static int            exit_code;
    bool                                deletable = false;
    std::mutex                          mx_self;
    std::function<void(var &)>         on_complete;
    std::function<void(var &)>         on_failure;
    std::function<var(Process *, int i)> fn;
    var                                data;
    std::vector<std::thread>            threads;
    int                                 completed = 0;
    bool                                failure   = false;
    bool                                join      = false;
    Process();
   ~Process();
};

struct Async {
    Process*        process;
    var            data;
    static void     thread(Process *, int i);
    static int      await();
    var&           result();
    Async          ();
    Async          (int c, std::function<var(Process *, int i)> f);
    Async          (const Async &r);
    Async&          operator=(const Async &r);
    Future          then(std::function<void(var &d)> fn);
   ~Async();
    operator Future();
    std::mutex      mx;
};

struct Customer {   std::function<void(var &)> fn;   };
struct CompleterData;
struct Completer;
struct Future   {
protected:
    CompleterData*  cdata;
public:
    Future&         then  (std::function<void(var &d)> fn);
    Future&         except(std::function<void(var &d)> fn);
    Future         (const Future &ref);
    Future         (Completer &c);
   ~Future         ();
    Future&         operator=(const Future &ref);
};

struct CompleterData {
    var            data;
    std::mutex      mx;
    bool            completed;
    vec<Customer>   l_success;
    vec<Customer>   l_failure;
};

struct Completer {
    CompleterData*  cdata;
    Completer(std::function<void(var &)> &fn_success, std::function<void(var &)> &fn_failure);
    virtual        ~Completer() { }
    operator        Future();
};
