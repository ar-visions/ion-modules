#pragma once
#include <ux/ux.hpp>
#include <ux/interface.hpp>
#include <web/web.hpp>
#include <thread>
#include <mutex>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

template <class V>
struct UX:Interface {
    struct Internal* intern;
    void set_title(str t);
    str         title;
    Composer *  composer;
    int         result = 0;
    /// -------------------------------------
    void run() {
        composer = Composer_init(this, fn, args);
        code     = Vulkan::main(composer);
    }
    /// -------------------------------------
    UX(int c, const char *v[], Args &defaults) {
        type     = Interface::UX;
        fn       = [&]() -> Element { return V(); };
        Interface::bootstrap(c, v, defaults);
    }
};

#include <ux/composer.hpp>
typedef std::function<bool (node *, node **, var &)> FnUpdate;

