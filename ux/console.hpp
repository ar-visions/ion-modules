#pragma once
#include <ux/ux.hpp>
#include <ux/interface.hpp>
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

/// this one people should really like. templated ux composition in a terminal.
/// why give up on the poor little terminal, why not use the same UX components?  hey!  [/hugs]
template <class V>
struct TX:Interface {
    array<ColoredGlyph>   cache;
    std::mutex          mx;
    /// ------------------------------------------------
    void run() {
        auto get_ch = []() {
            #if defined(UNIX)
                return getchar();
            #else
                return getch_();
            #endif
        };
        Composer *cc = Composer_init(this, args);
        ///
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
        ///
        for (;;) {
            bool resized = wsize_change(0);
            if (resized)
                continue;
            char c = get_ch();
            if (uint8_t(c) == 0xff)
                wsize_change(0);
            std::cout << "\x1b[2J\x1b[1;1H" << std::flush;
            //system("clear");
            str ch = c;
            Composer_render(cc, fn);
            Composer_input (cc, ch);
        }
    }

    static bool wsize_change(int sig) {
        static winsize ws;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
        int sx = ws.ws_col;
        int sy = ws.ws_row;
        TX &app = *(TX *)instance;
        bool resized = app.sz.x != sx || app.sz.y != sy;
        if (resized) {
            if (sig != 0)
                app.mx.lock();
            if (sx == 0) {
                sx = 80;
                sy = 24;
            }
            app.sz     = {sx, sy};
            app.canvas = Canvas(app.sz, Canvas::Terminal);
            if (sig != 0)
                app.mx.unlock();
        }
        return resized;
    }
    
    void draw(node *root) {
        Interface::draw(root);
        array<ColoredGlyph> *buf = (array<ColoredGlyph> *)canvas.data();
        bool update_cache = !cache || cache.size() != buf->size();
        /// update on resize, and update these buffers
        if (update_cache) {
            cache = array<ColoredGlyph>(buf->size(), null);
            //for (size_t i = 0; i < sz.y; i++)
            //    fprintf(stdout, "\n");
        }
        assert(sz.x * sz.y == int(buf->size()));
        assert(sz.x * sz.y == int(cache.size()));
        for (size_t iy = 0; iy < size_t(sz.y); iy++) {
            for (size_t ix = 0; ix < size_t(sz.x); ix++) {
                size_t index = iy * size_t(sz.x) + ix;
                ColoredGlyph &n = (*buf)[index];
                ColoredGlyph &c =  cache[index];
                assert(index < buf->size());
                if (update_cache || n != c) {
                    /// print colored ansi text at xy
                    auto ansi = [&](ColoredGlyph &n, std::string s) -> std::string {
                        if (!n.bg && !n.fg)
                            return s;
                        auto str_bg = canvas.ansi_color(n.bg, false);
                        auto str_fg = canvas.ansi_color(n.fg, true);
                        return str_bg.s + str_fg.s + s + "\u001b[0m";
                    };
                    auto ch = canvas.get_char(ix, iy);
                    auto  a = ansi(n, ch);
                    fprintf(stdout, "\033[%zu;%zuH%s", iy + 1, ix + 1, a.c_str());
                    /// update cache
                    c = n;
                }
            }
        }
        fflush(stdout);
    }
    /// ------------------------------------------------
    TX(int c, cchar_t *v[], Map &defaults) {
        type    = Interface::TX;
        fn      = []() -> Element { return V(); };
        Interface::bootstrap(c, v, defaults);
    }
};



