#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <thread>
#include <vk/vk.hpp>
#include <dx/dx.hpp>
#include <ux/ux.hpp>
#include <web/web.hpp>

int App::UX::operator()(FnRender fn) {
    return Vulkan::main(fn, composer);
}

App::UX::UX(int c, const char *v[], Args &defaults) {
    type = Interface::UX;
    args = var::args(c, v);
    for (auto &[k,v]: defaults)
        if (args.count(k) == 0)
            args[k] = v;
    composer = new Composer(this, args);
}

App::Server::Server(int c, const char *v[], Args &defaults) {
    type = Interface::Server;
    args = var::args(c, v);
    for (auto &[k,v]: defaults) {
        if (args.count(k) == 0)
            args[k] = v;
    }
    args.erase("server-port");
    str uri = "https://0.0.0.0:443";
    /// create new composer -- multiple roots should be used
    /// less we use the tree as a controller and filter only
    async = Web::server(uri, [&](Web::Message &m) -> Web::Message {
        /// figure out the args here
        Args a;
        var p = null;
        return query(m, a, p);
    });
}

Web::Message App::Server::query(Web::Message &m, Args &a, var &p) {
    Web::Message msg;
    auto r = m.uri.resource;
    auto sp = m.uri.resource.split("/");
    auto sz = sp.size();
    if (sz < 2 || sp[0])
        return {400, "bad request"};
    
    /// route/to/data#id
    Composer cc = Composer((Interface *)this, a);
    
    /// instantiate components -- this may be in session cache
    cc(fn(a));
    
    node     *n = cc.root;
    str      id = sp[1];
    for (size_t i = 1; i < sz; i++) {
        n = n->mounts[id];
        if (!n)
            return {404, "not found"};
    }
    return msg;
}

int App::Server::operator()(FnRender fn) {
    this->fn = fn;
    return async.await();
}

static App::Console *g_this = nullptr;

App::Console::Console() {
    args   = null;
    g_this = this;
}

App::Console::Console(int c, const char *v[]) {
    args   = var::args(c, v);
    g_this = this;
}

#if defined(UNIX)
char getch_() {
    return getchar();
}
#endif

int App::Console::operator()(FnRender fn) {
    Args args;
    assert(g_this);
    Composer cc         = Composer((Interface *)this, args);
#if defined(UNIX)
    struct termios t = {};
    tcgetattr(0, &t);
    t.c_lflag       &= ~ICANON;
    t.c_lflag       &= ~ECHO;
    tcsetattr(0, TCSANOW, &t);
#else
    HANDLE   h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    uint32_t mode    = 0;
    GetConsoleMode(h_stdin, &mode);
    SetConsoleMode(h_stdin, mode & ~ENABLE_ECHO_INPUT);
#endif
    for (;;) {
        bool resized = wsize_change(0);
        if (resized)
            continue;
        char c = getch_();
        if (uint8_t(c) == 0xff)
            wsize_change(0);
        std::cout << "\x1b[2J\x1b[1;1H" << std::flush;
        //system("clear");
        cc(fn(args));
        cc.input(c);
    }
    return 0;
}

bool App::Console::wsize_change(int sig) {
    static winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    int sx = ws.ws_col;
    int sy = ws.ws_row;
    bool resized = g_this->sz.x != sx || g_this->sz.y != sy;
    if (resized) {
        if (sig != 0)
            g_this->mx.lock();
        if (sx == 0) {
            sx = 80;
            sy = 24;
        }
        g_this->sz     = {sx, sy};
        g_this->canvas = Canvas(g_this->sz, Canvas::Terminal);
        if (sig != 0)
            g_this->mx.unlock();
    }
    return resized;
}

