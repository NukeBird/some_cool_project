#include "glfw_window.h"
#include <mutex>
#include <cassert>

namespace
{
    class GlfwKeeper
    {
    public:
        std::mutex mutex;

        static GlfwKeeper& Get()
        {
            static GlfwKeeper keeper;
            return keeper;
        }

        GlfwKeeper(const GlfwKeeper&) = delete;
        GlfwKeeper(GlfwKeeper&&) = delete;
        GlfwKeeper& operator=(const GlfwKeeper&) = delete;
        GlfwKeeper& operator=(GlfwKeeper&&) = delete;

    private:
        GlfwKeeper()
        {
            if (!glfwInit())
                throw GlfwException("Initialization failed");

            glfwSetErrorCallback([](int, const char* message) {
                throw GlfwException(message);
                });
        }

        ~GlfwKeeper()
        {
            glfwTerminate();
        }
    };
}

GlfwWindow* GlfwWindow::FromNativeWindow(const GLFWwindow* window)
{
    return static_cast<GlfwWindow*>(glfwGetWindowUserPointer(const_cast<GLFWwindow*>(window)));
}

void GlfwWindow::TryUpdateGlBindings()
{
    globjects::init([](const char* name) 
    {
        return glfwGetProcAddress(name);
    });
}

GlfwWindow::~GlfwWindow()
{
    std::lock_guard lock(GlfwKeeper::Get().mutex);

    glfwDestroyWindow(window_impl);
}

void GlfwWindow::Run()
{
    {
        std::lock_guard lock(GlfwKeeper::Get().mutex);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OpenGL_Context_Version_Major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OpenGL_Context_Version_Minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, num_msaa_samples);

        /* Create a windowed mode window and its OpenGL context */
        window_impl = glfwCreateWindow(window_size.x, window_size.y, window_label.c_str(), nullptr, nullptr);
        if (!window_impl)
            throw GlfwException("Windows creation failed");

        glfwSetWindowUserPointer(window_impl, this);

        SetupCallbacks();
    }

    MakeContextCurrent();
    OnInitialize();
    OnResize(getSize());

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window_impl))
    {
        Render();
        glfwPollEvents();
    }
}

void GlfwWindow::Render()
{
    MakeContextCurrent();

    {
        double currentTime = glfwGetTime();
        OnRender(currentTime, currentTime - previous_render_time);
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

GlfwWindow& GlfwWindow::setNumMsaaSamples(int samples)
{
    if (!window_impl)
        num_msaa_samples = samples;
    else
        throw GlfwException("MSAA could be only setted before Run() call");

    return *this;
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
        FromNativeWindow(window)->OnMouseMove(glm::vec2(x, y));
    });
    
    glfwSetFramebufferSizeCallback(window_impl, [](GLFWwindow *window, int w, int h) {
        FromNativeWindow(window)->OnResize(glm::ivec2(w, h));
    });

    glfwSetWindowCloseCallback(window_impl, [](GLFWwindow *window) {
        FromNativeWindow(window)->OnClosing();
    });

    glfwSetWindowRefreshCallback(window_impl, [](GLFWwindow *window) {
        auto wnd = FromNativeWindow(window);
        wnd->OnWindowRefreshing();
        wnd->Render();
    });

    glfwSetScrollCallback(window_impl, [](GLFWwindow* window, double xoffset, double yoffset) {
        FromNativeWindow(window)->OnMouseScroll(glm::vec2(xoffset, yoffset));
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