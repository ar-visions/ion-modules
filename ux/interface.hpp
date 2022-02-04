#pragma once
#include <ux/ux.hpp>
#include <web/web.hpp>
#include <thread>
#include <mutex>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

struct AppInternal;
int run(AppInternal **, var &);

struct Composer;
struct var;

Composer *Composer_init(void *ix, FnRender fn, Map &args);
node     *Composer_root(Composer *cc);
void      Composer_render(Composer *cc, FnRender &fn);
void      Composer_input(Composer *cc, str &s);


struct Interface {
    enum Type {
        Undefined,
        UX, DX, TX
    }           type = Undefined;
    vec2i       sz   = {0,0};
    Map         args;
    Canvas      canvas;
    FnRender    fn;
    int         code = 0; /// result code
    ///
    operator int();
    /// ------------------------------------------------
    void bootstrap(int c, cchar_t *v[], Map &defaults) {
        args = var::args(c, v);
        for (auto &[k,v]: defaults)
            if (args.count(k) == 0)
                args[k] = v;
        run();
    }
    static Interface *instance;
    virtual void run() { }
    virtual void draw(node *root);
    Interface(Type type) : type(type) { };
    Interface(std::nullptr_t n = nullptr) : type(Undefined) { }
};

