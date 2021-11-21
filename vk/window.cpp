#include <vulkan/vulkan.hpp>
#include <data/data.hpp>
#include <media/canvas.hpp>
#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk/window.hpp>

// all clipboard has to be pass-through to Vulkan which sucks.
// has to be done, window must be hidden!... we dont use UX.
// want to completely hide GLFW, and it is the case because Window is internal to Vulkan
static void glfw_error  (int code, const char *cstr) {
    console.log("glfw error: {0}", {str(cstr)});
}

Window &Window::ref(GLFWwindow *h) {
    return *(Window *)glfwGetWindowUserPointer(h);
}

static void glfw_key    (GLFWwindow *h, int key, int scancode, int action, int mods) {
    auto win = Window::ref(h);
    if (win.fn_key)
        win.fn_key(key, scancode, action, mods);
}

static void glfw_char   (GLFWwindow *h, uint32_t code) {
    auto win = Window::ref(h);
    if (win.fn_char)
        win.fn_char(code);
}

static void glfw_mbutton(GLFWwindow *h, int button, int action, int mods) {
    auto win = Window::ref(h);
    if (win.fn_mbutton)
        win.fn_mbutton(button, action, mods);
}

static void glfw_cursor (GLFWwindow *handle, double x, double y) {
    auto win = Window::ref(handle);
    if (win.fn_cursor)
        win.fn_cursor(x, y);
}

static void glfw_resize (GLFWwindow *handle, int32_t w, int32_t h) {
    auto    win = Window::ref(handle);
    win.size    = vec2i { w, h };
    if (win.fn_resize)
        win.fn_resize();
}

Window::operator GLFWwindow *() {
    return handle;
}

str Window::clipboard() {
    return glfwGetClipboardString(handle);
}

void Window::set_clipboard(str text) {
    glfwSetClipboardString(handle, text.cstr());
}

void Window::set_title(str s) {
    title = s;
    glfwSetWindowTitle(handle, title.cstr());
}

///
/// This is the show window method
void Window::show() {
    glfwShowWindow(handle);
}

///
/// This is the hide window method
void Window::hide() {
    glfwHideWindow(handle);
}

///
/// Window Constructors
Window::Window(nullptr_t n) { }
Window::Window(vec2i     sz) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    handle = glfwCreateWindow(sz.x, sz.y, title.cstr(), nullptr, nullptr);
    ///
    glfwSetWindowUserPointer      (handle, this);
    glfwSetWindowUserPointer      (handle, this);
    glfwSetFramebufferSizeCallback(handle, glfw_resize);
    glfwSetKeyCallback            (handle, glfw_key);
    glfwSetCursorPosCallback      (handle, glfw_cursor);
    glfwSetCharCallback           (handle, glfw_char);
    glfwSetMouseButtonCallback    (handle, glfw_mbutton);
}

///
/// Window Destructor
Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

