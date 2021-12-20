#include <vulkan/vulkan.hpp>
#include <dx/dx.hpp>
#include <media/canvas.hpp>
#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk/vk.hpp>
#include <vk/window.hpp>

static void glfw_error  (int code, const char *cstr) {
    console.log("glfw error: {0}", {str(cstr)});
}

Window &Window::ref(GLFWwindow *h) {
    Window *w = (Window *)glfwGetWindowUserPointer(h);
    return *w;
}

static void glfw_key    (GLFWwindow *h, int key, int scancode, int action, int mods) {
    auto &win = Window::ref(h);
    if (win.fn_key)
        win.fn_key(key, scancode, action, mods);
}

static void glfw_char   (GLFWwindow *h, uint32_t code) {
    auto &win = Window::ref(h);
    if (win.fn_char)
        win.fn_char(code);
}

static void glfw_mbutton(GLFWwindow *h, int button, int action, int mods) {
    auto &win = Window::ref(h);
    if (win.fn_mbutton)
        win.fn_mbutton(button, action, mods);
}

static void glfw_cursor (GLFWwindow *handle, double x, double y) {
    auto &win = Window::ref(handle);
    if (win.fn_cursor) {
        win.fn_cursor(x, y);
    }
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

Window::operator VkSurfaceKHR() {
    if (!surface) {
        auto   vk = Vulkan::instance();
        auto code = glfwCreateWindowSurface(vk, handle, null, &surface);
        assert(code == VK_SUCCESS);
    }
    return surface;
}

void Window::set_title(str s) {
    title = s;
    glfwSetWindowTitle(handle, title.cstr());
}

void Window::show() {
    glfwShowWindow(handle);
}

void Window::hide() {
    glfwHideWindow(handle);
}

Window *Window::first = null;

/// Window Constructors
Window::Window(nullptr_t n) { }
Window::Window(vec2i     sz): size(sz) {
    handle = glfwCreateWindow(sz.x, sz.y, title.cstr(), nullptr, nullptr);
    ///
    glfwSetWindowUserPointer      (handle, this);
    glfwSetFramebufferSizeCallback(handle, glfw_resize);
    glfwSetKeyCallback            (handle, glfw_key);
    glfwSetCursorPosCallback      (handle, glfw_cursor);
    glfwSetCharCallback           (handle, glfw_char);
    glfwSetMouseButtonCallback    (handle, glfw_mbutton);
    glfwSetErrorCallback          (glfw_error);
}

int Window::loop(std::function<void()> fn) {
    /// render loop with node event processing
    while (!glfwWindowShouldClose(handle)) {
        fn();
        glfwPollEvents();
    }
    return 0;
}

///
/// Window Destructor
Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

