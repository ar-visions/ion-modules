#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <thread>
#include <vk/vk.hpp>
#include <data/data.hpp>
#include <ux/ux.hpp>
#include <web/web.hpp>

#define  vk_load(inst, addr) PFN_##addr addr = (PFN_##addr)glfwGetInstanceProcAddress(inst, #addr);

struct GLFWwindow;

static GLFWwindow *g_glfw;

struct WindowInternal {
protected:
    static void glfw_error  (int, const char *) {
        console.log("glfw error");
    }
    
    static void glfw_key    (GLFWwindow*, int, int, int, int) {
        console.log("glfw key");
    }
    
    static void glfw_char   (GLFWwindow*, uint32_t) {
        console.log("glfw char");
    }
    
    static void glfw_mbutton(GLFWwindow*, int, int, int) {
        console.log("glfw mbutton");
    }
    
    static void glfw_cursor (GLFWwindow*, double x, double y) {
        console.log("glfw cursor: {0}, {1}", { x, y });
    }
    
    static void glfw_resize (GLFWwindow*, int32_t, int32_t) {
        console.log("glfw resize");
    }
    
public:
    GLFWwindow  *glfw;
    Data         mouse     = Args {};
    std::string  title     = "";
    vec2         dpi_scale = { 1, 1 };
    Vec2<size_t> size      = { 0, 0 };
    bool         repaint   = false;
    KeyStates    k_states  = { };
    
    std::string clipboard();
    void set_clipboard(std::string text);
    WindowInternal(std::string title, Vec2<size_t> size) : title(title), size(size) {
        assert(g_glfw == null);
        
        /*
        glfwInit();
        
        // **important** create window without initializing GL subsystem
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
          glfw = glfwCreateWindow(size.x, size.y, title.c_str(), null, null);
        g_glfw = glfw;
        */
        
        glfw   = Vulkan::get_window();
        g_glfw = glfw;
        
        
        //Vulkan::init();
        //Vulkan::resize(size.x, size.y);
        /*
        glfwSetWindowUserPointer      (glfw, this);
        glfwSetFramebufferSizeCallback(glfw, glfw_resize);
        glfwSetKeyCallback            (glfw, glfw_key);
        glfwSetCursorPosCallback      (glfw, glfw_cursor);
        glfwSetCharCallback           (glfw, glfw_char);
        glfwSetMouseButtonCallback    (glfw, glfw_mbutton);*/
    }
    ~WindowInternal() {
        glfwDestroyWindow(glfw);
        glfwTerminate();
    }
    void set_title(str title) {
        glfwSetWindowTitle(glfw, title.cstr());
    }
    void init_cursor();
    bool resize();
    bool mouse_down(MouseButton button);
};

GLFWwindow *App::Window::handle() { return g_glfw; }

void App::Window::set_title(str t) {
    if (t != title) {
        intern->set_title(t);
        title = t;
    }
}

int App::Window::operator()(FnRender fn) {
    /// allocate composer, the class which takes rendered elements and manages instances
    //auto composer = Composer(*this);
    
    /// allocate window internal (glfw subsystem) and canvas
    intern = new WindowInternal("", {size_t(sz.x), size_t(sz.y)});
    canvas = Canvas({int(sz.x), int(sz.y)}, Canvas::Context2D);
    
    /// render loop with node event processing
    while (!glfwWindowShouldClose(intern->glfw)) {
        rectd box   = {   0,   0, 320, 240 };
        rgba  color = { 255, 255,   0, 255 };
        
        canvas.color(color);
        canvas.fill(box);
        canvas.flush();
        //Vulkan::swap(sz);
        
        // glfw might just manage swapping? but lets remove it for now
        //glfwSwapBuffers(intern->glfw);
        glfwWaitEventsTimeout(1.0);
        /*
        composer(fn(args));
        set_title(composer.root->props.text.label);
        if (!composer.process())
            glfwWaitEventsTimeout(1.0);
        */
        glfwPollEvents();
    }
    delete intern;
    return 0;
}

App::Window::Window(int c, const char *v[], Args &defaults) {
    type = Interface::Window;
    args = Data::args(c, v);
    for (auto &[k,v]: defaults) {
        if (args.count(k) == 0)
            args[k] = v;
    }
    /// init a valid size, the view does not dictate this but it could end up saving a persistent arg
    sz = {args["window-width"],
          args["window-height"]};
    if (sz.x == 0 || sz.y == 0)
        sz = {800, 800};
    args.erase("window-width");
    args.erase("window-height");
}

App::Server::Server(int c, const char *v[], Args &defaults) {
    type = Interface::Server;
    args = Data::args(c, v);
    for (auto &[k,v]: defaults) {
        if (args.count(k) == 0)
            args[k] = v;
    }
    args.erase("server-port");
    str uri = "https://0.0.0.0:443";
    /// create new composer -- multiple roots should be used less we use the tree as a controller and filter only
    async = Web::server(uri, [&](Web::Message &m) -> Web::Message {
        /// figure out the args here
        Args a;
        Data p = null;
        return query(m, a, p);
    });
}

Web::Message App::Server::query(Web::Message &m, Args &a, Data &p) {
    Web::Message msg;
    auto r = m.uri.resource;
    auto sp = m.uri.resource.split("/");
    auto sz = sp.size();
    if (sz < 2 || sp[0])
        return {400, "bad request"};
    
    // route/to/data#id
    
    Composer cc = Composer((Interface *)this);
    
    /// instantiate components -- this may be in session cache
    cc(fn(a));
    
    node     *n = cc.root;
    str      id = sp[1];
    for (size_t i = 1; i < sz; i++) {
        n = n->mounts[id];
        if (!n)
            return {404, "not found"};
    }
    // how does one go from a node to the data associated?  these nodes are there for selection but they have to link to data
    // one idea was to use the idea of 'state', call it 'model' instead.
    // now query this node
    
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
    args   = Data::args(c, v);
    g_this = this;
}

#if defined(UNIX)
char getch_() {
    return getchar();
}
#endif

int App::Console::operator()(FnRender fn) {
    assert(g_this);
    Composer cc         = Composer((Interface *)this);
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

