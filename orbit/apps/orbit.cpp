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
#include <ux/interface.hpp>
#include <ux/composer.hpp>

Interface *Interface::instance = nullptr;
///
Interface::operator int() { return code; }
///
Composer *Composer_init(void *ix, FnRender fn, Args &args) {
    return new Composer((Interface *)ix, fn, args);
}
///
node *Composer_root(Composer *cc)         { return cc->root; }
///
void Composer_render(Composer *cc)        { cc->render(); }
///
void Composer_input(Composer *cc, str &s) { cc->input(s); }
///
/// render is popular because it pushes the act of rendition upwards to elements and their binds
/// the next thing up from that is composition, it takes care of groups of components in its abstract
void Interface::draw(node *root) {
    /// setup recursion lambda
    std::function<void(node *)> recur; recur = [&](node *n) {
        canvas.save();
        canvas.cap(n->m.border.cap());
        canvas.join(n->m.border.join());
        canvas.save();
        n->draw(canvas);
        canvas.clip(n->paths.fill);
        for (auto &[id, mounted]: n->mounts) {
            vec2 xy = n->paths.rect.xy();
            canvas.translate(xy);
            // this is where we clip and such
            recur(mounted);
        }
        canvas.restore();
        canvas.restore();
    };
    recur(root);
}
