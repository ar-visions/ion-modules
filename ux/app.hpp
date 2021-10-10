#pragma once
#include <ux/ux.hpp>
#include <web/web.hpp>
#include <thread>
#include <mutex>

struct AppInternal;
int run(AppInternal **, Data &);

enum KeyState {
    KeyUp,
    KeyDown
};

struct KeyStates {
    bool shift;
    bool ctrl;
    bool meta;
};

struct KeyInput {
    int key;
    int scancode;
    int action;
    int mods;
};

enum MouseButton {
    LeftButton,
    RightButton,
    MiddleButton
};

enum KeyCode {
    D           = 68,
    N           = 78,
    Backspace   = 8,
    Tab         = 9,
    LineFeed    = 10,
    Return      = 13,
    Shift       = 16,
    Ctrl        = 17,
    Alt         = 18,
    Pause       = 19,
    CapsLock    = 20,
    Esc         = 27,
    Space       = 32,
    PageUp      = 33,
    PageDown    = 34,
    End         = 35,
    Home        = 36,
    Left        = 37,
    Up          = 38,
    Right       = 39,
    Down        = 40,
    Insert      = 45,
    Delete      = 46, // or 127 ?
    Meta        = 91
};

struct Composer;
struct GLFWwindow;
struct Data;
struct WindowInternal;

struct App {
    struct Interface {
        enum Type {
            Undefined,
            Server,
            Console,
            Window
        }      type = Undefined;
        vec2i  sz   = {0,0};
        Args   args;
        Canvas canvas;
        Interface(Type type) : type(type) { };
        Interface(nullptr_t n = nullptr) : type(Undefined) { }
        virtual void draw(node *root) {
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
    };
    
    struct Window:Interface {
        struct WindowInternal* intern;
        static GLFWwindow *handle();
        Window(int c, const char *v[], Args &defaults);
        int operator()(FnRender fn);
        void set_title(str t);
        str title;
    };

    struct Server:Interface {
        Async async;
        FnRender fn;
        struct ServerInternal* intern;
        Server(int c, const char *v[], Args &defaults);
        int operator()(FnRender fn);
        Web::Message query(Web::Message &m, Args &a, Data &p);
    };

    /// todo: fix up scroll-related issues in terminal rendering
    struct Console:Interface {
        vec<ColoredGlyph> cache;
        std::mutex mx;
        static bool wsize_change(int sig);
        Console();
        Console(int c, const char *v[]);
        
        int operator()(FnRender fn);
        
        void draw(node *root) {
            Interface::draw(root);
            vec<ColoredGlyph> *buf = (vec<ColoredGlyph> *)canvas.data();
            bool update_cache = !cache || cache.size() != buf->size();
            
            /// update on resize, and update these buffers
            if (update_cache) {
                cache = vec<ColoredGlyph>(buf->size());
                cache.fill(null);
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
    };
};

#include <ux/composer.hpp>
typedef std::function<bool (node *, node **, Data &)> FnUpdate;
