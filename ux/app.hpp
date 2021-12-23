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

Composer *Composer_init(void *ix, Args &args);
node     *Composer_root(Composer *cc);
void      Composer_render(Composer *cc, FnRender &fn);
void      Composer_input(Composer *cc, str &s);

struct App {
    struct Interface {
        enum Type {
            Undefined,
            UX, DX, TX
        }           type = Undefined;
        vec2i       sz   = {0,0};
        Args        args;
        Canvas      canvas;
        FnRender    fn;
        int         code = 0; /// result code
        ///
        operator int();
        /// ------------------------------------------------
        void bootstrap(int c, const char *v[], Args &defaults) {
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
        Interface(nullptr_t n = nullptr) : type(Undefined) { }
    };
    
    template <class V>
    struct UX:Interface {
        struct Internal* intern;
        void set_title(str t);
        str         title;
        Composer *  composer;
        int         result = 0;
        /// -------------------------------------
        void run() {
            composer = Composer_init(this, args);
            code     = Vulkan::main(fn, composer);
        }
        /// -------------------------------------
        UX(int c, const char *v[], Args &defaults) {
            type     = Interface::UX;
            fn       = [&]() -> Element { return V(); };
            Interface::bootstrap(c, v, defaults);
        }
    };

    template <class V>
    struct DX:Interface {
        Async       async;
        struct ServerInternal* intern;
        /// ------------------------------------------------
        Web::Message query(Web::Message &m, Args &a, var &p) {
            Web::Message msg;
            auto r = m.uri.resource;
            auto sp = m.uri.resource.split("/");
            auto sz = sp.size();
            if (sz < 2 || sp[0])
                return {400, "bad request"};
            
            /// route/to/data#id
            Composer *cc = Composer_init(this, a);
            
            /// instantiate components -- this may be in session cache
            (*cc)(Interface::fn());
            
            node     *n = Composer_root(cc);
            str      id = sp[1];
            for (size_t i = 1; i < sz; i++) {
                n = n->mounts[id];
                if (!n)
                    return {404, "not found"};
            }
            return msg;
        }
        /// ------------------------------------------------
        void run() {
            str uri = "https://0.0.0.0:443";
            async   = Web::server(uri, [&](Web::Message &m) -> Web::Message {
                Args a;
                var p = null;
                return query(m, a, p);
            });
            code = async.await();
        }
        /// ------------------------------------------------
        template <typename C>
        DX(int c, const char *v[], Args &defaults) {
            type    = Interface::DX;
            fn      = []() -> Element { return C(); };
            Interface::bootstrap(c, v, defaults);
        }
    };

    /// this one people should really like. templated ux composition in a terminal.
    /// why give up on the poor little terminal, why not use the same UX components?  hey!  [/hugs]
    template <class V>
    struct TX:Interface {
        vec<ColoredGlyph>   cache;
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
        
        /// split these up into tx, ux and dx
        void draw(node *root) {
            Interface::draw(root);
            vec<ColoredGlyph> *buf = (vec<ColoredGlyph> *)canvas.data();
            bool update_cache = !cache || cache.size() != buf->size();
            /// update on resize, and update these buffers
            if (update_cache) {
                cache = vec<ColoredGlyph>(buf->size(), null);
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
        TX(int c, const char *v[], Args &defaults) {
            type    = Interface::TX;
            fn      = []() -> Element { return V(); };
            Interface::bootstrap(c, v, defaults);
        }
    };
};

#include <ux/composer.hpp>
typedef std::function<bool (node *, node **, var &)> FnUpdate;

