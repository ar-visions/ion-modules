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

App::Interface *App::Interface::instance = nullptr;

App::Interface::operator int() {
    return code;
}

void App::Interface::draw(node *root) {
    /// setup recursion lambda
    std::function<void(node *)> recur;
                                recur = [&](node *n) {
        canvas.save();
        canvas.translate(n->path.xy());
        n->draw(canvas);
        for (auto &[id, m]: n->mounts)
            recur(m);
        canvas.restore();
    };
    recur(root);
}

Composer *Composer_init(void *ix, Args &args) {
    return new Composer((App::Interface *)ix, args);
}

node *Composer_root(Composer *cc) {
    return cc->root;
}

void Composer_render(Composer *cc, FnRender &fn) {
    (*cc)(fn());
}

void Composer_input(Composer *cc, str &s) {
    cc->input(s);
}
