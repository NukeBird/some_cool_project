#include "glfw_window.h"
#include <mutex>
#include <cassert>

#include "glfw_application.h"


GlfwWindow* GlfwWindow::FromNativeWindow(const GLFWwindow* window)
{
    const auto frontendPtr = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(const_cast<GLFWwindow*>(window)));
    assert(frontendPtr);
    return frontendPtr;
}

void GlfwWindow::TryUpdateGlBindings()
{
    globjects::init([](const char* name) 
    {
        return glfwGetProcAddress(name);
    });
}

GlfwWindow::GlfwWindow(GlfwContextParameters contextParams) :
    context_parameters(contextParams)
{
    //std::lock_guard lock(GlfwApplication::Get().mutex);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, context_parameters.context_major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, context_parameters.context_minor_version);
    glfwWindowHint(GLFW_OPENGL_PROFILE, static_cast<int>(context_parameters.profile));
    glfwWindowHint(GLFW_SAMPLES, context_parameters.msaa_samples);

    /* Create a windowed mode window and its OpenGL context */
    window_impl = glfwCreateWindow(window_size.x, window_size.y, window_label.c_str(), nullptr, nullptr);
    if (!window_impl)
        throw GlfwException("Windows creation failed");

    glfwSetWindowUserPointer(window_impl, this);

    SetupCallbacks();
}

bool GlfwWindow::isKeyDown(int keyCode) const
{
    return window_impl && (glfwGetKey(window_impl, keyCode) == GLFW_PRESS);
}

bool GlfwWindow::isMouseKeyDown(int button) const
{
    return window_impl && (glfwGetMouseButton(window_impl, button) == GLFW_PRESS);
}

GlfwWindow& GlfwWindow::setCursorMode(GlfwCursorMode cursorMode)
{
    if (window_impl)
        glfwSetInputMode(window_impl, GLFW_CURSOR, static_cast<int>(cursorMode));

    return *this;
}

void GlfwWindow::UpdateAndRender()
{
    assert(window_impl);
    MakeContextCurrent();

    {
        if (!isInitialized)
        {
            isInitialized = true;
            OnInitialize();
            OnResize(getSize());
        }

        const double currentTime = glfwGetTime();
        OnUpdate(currentTime - previous_render_time);
        OnRender(currentTime - previous_render_time);
        previous_render_time = currentTime;
    }

    glfwSwapBuffers(window_impl);
}

GlfwWindow& GlfwWindow::setLabel(const std::string& label)
{
    window_label = label;
    if (window_impl != nullptr)
        glfwSetWindowTitle(window_impl, label.c_str());

    return *this;
}

//TODO: find a way to invoke it indirectly? Also need to remake anyway.
GlfwWindow& GlfwWindow::setVSyncInterval(int interval)
{
    MakeContextCurrent();
    glfwSwapInterval(interval);

    return *this;
}

GlfwWindow& GlfwWindow::setCloseFlag(bool flag)
{
    MakeContextCurrent();
    glfwSetWindowShouldClose(window_impl, flag ? GLFW_TRUE : GLFW_FALSE);

    //When flag is setted manually there is no callback, so we should call it manually
    if (flag)
        OnClosing();

    return *this;
}

bool GlfwWindow::getCloseFlag() const
{
    if (window_impl)
        return glfwWindowShouldClose(window_impl);

    return false;
}

void GlfwWindow::requestAttention()
{
    if (window_impl)
        glfwRequestWindowAttention(window_impl);
}

GlfwWindow& GlfwWindow::setSize(int width, int height)
{
    window_size = glm::ivec2(width, height);

    if (window_impl != nullptr)
        glfwSetWindowSize(window_impl, window_size.x, window_size.y);

    return *this;
}

void GlfwWindow::SetupCallbacks()
{

    glfwSetKeyCallback(window_impl, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto wnd = GlfwWindow::FromNativeWindow(window);

        switch (action)
        {
        case GLFW_RELEASE:
            wnd->OnKeyUp(key); break;
        case GLFW_PRESS:
            wnd->OnKeyDown(key); break;
        case GLFW_REPEAT:
            wnd->OnKeyRepeat(key); break;
        default:
            throw GlfwException("Unknown key action");
        }
        
    });

    glfwSetMouseButtonCallback(window_impl, [](GLFWwindow* window, int button, int action, int mods) {
        auto wnd = FromNativeWindow(window);
        if (action == GLFW_PRESS)
            wnd->OnMouseDown(button);
        else if (action == GLFW_RELEASE)
            wnd->OnMouseUp(button);
    });

    glfwSetCursorPosCallback(window_impl, [](GLFWwindow* window, double x, double y) {
        FromNativeWindow(window)->OnMouseMove({ x, y });
    });
    
    glfwSetFramebufferSizeCallback(window_impl, [](GLFWwindow *window, int w, int h) {
        FromNativeWindow(window)->OnResize({ w, h });
    });

    glfwSetWindowCloseCallback(window_impl, [](GLFWwindow *window) {
        FromNativeWindow(window)->OnClosing();
    });

    glfwSetWindowRefreshCallback(window_impl, [](GLFWwindow *window) {
        auto wnd = FromNativeWindow(window);
        wnd->OnWindowRefreshing();
        //wnd->UpdateAndRender();
    });

    glfwSetScrollCallback(window_impl, [](GLFWwindow* window, double offsetX, double offsetY) {
        FromNativeWindow(window)->OnMouseScroll({ offsetX, offsetY });
    });
    
}

void GlfwWindow::MakeContextCurrent()
{
    GLFWwindow* actualContext = glfwGetCurrentContext();

    if (actualContext != window_impl)
    {
        glfwMakeContextCurrent(window_impl);

        //in common case gl functions must be updated after context switching, but fortunately in case "one context per one thread" it don't occured
        TryUpdateGlBindings();
    }
}

void GlfwWindow::Close()
{
    //std::lock_guard lock(GlfwApplication::Get().mutex);

    glfwDestroyWindow(window_impl);
    window_impl = nullptr;
}
