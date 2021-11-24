#pragma once
#include <functional>
#include <dx/dx.hpp>

struct GLFWwindow;
struct Window {
protected:
    GLFWwindow *handle;
public:
    var          mouse     = Args {};
    str          title     = "";
    vec2         dpi_scale = { 1, 1 };
    vec2i        size      = { 0, 0 };
    bool         repaint   = false;
    KeyStates    k_states  = { };
    
    std::function<void()>                   fn_resize;
    std::function<void(uint32_t)>           fn_char;
    std::function<void(int, int, int)>      fn_mbutton;
    std::function<void(double, double)>     fn_cursor;
    std::function<void(int, int, int, int)> fn_key;
    
    static Window &ref(GLFWwindow *);
    static void on_resize(GLFWwindow *, int, int);
    void        set_title(str);
    str         clipboard();
    void        set_clipboard(str);
    void        show();
    void        hide();
    
    static Window *first;
    
    Window(vec2i);
   ~Window();
    Window(nullptr_t = null);
    operator GLFWwindow *();
};
