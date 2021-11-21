#pragma once
#include <ux/ux.hpp>
#include <web/web.hpp>
#include <thread>
#include <mutex>

struct AppInternal;
int run(AppInternal **, var &);

struct Composer;
struct var;

struct App {
    struct Interface {
        enum Type {
            Undefined,
            Server,
            Console,
            UX
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
    
    struct UX:Interface {
        struct Internal* intern;
        UX(int c, const char *v[], Args &defaults);
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
        Web::Message query(Web::Message &m, Args &a, var &p);
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
typedef std::function<bool (node *, node **, var &)> FnUpdate;

