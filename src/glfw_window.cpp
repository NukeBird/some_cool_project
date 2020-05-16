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

void GlfwWindow::Run()
{
    {
        std::lock_guard lock(GlfwKeeper::Get().mutex);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OpenGL_Context_Version_Major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OpenGL_Context_Version_Minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 8);

        /* Create a windowed mode window and its OpenGL context */
        m_window = glfwCreateWindow(m_windowSize.x, m_windowSize.y, m_windowLabel.c_str(), nullptr, nullptr);
        if (!m_window)
            throw GlfwException("Windows creation failed");

        glfwSetWindowUserPointer(m_window, this);

        SetupCallbacks();
    }

    MakeContextCurrent();
    OnInitialize();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(m_window))
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
        OnRender(currentTime, currentTime - m_previousRenderTime);
        m_previousRenderTime = currentTime;
    }

    glfwSwapBuffers(m_window);
}

GlfwWindow& GlfwWindow::setWindowLabel(const std::string& label)
{
    m_windowLabel = label;
    if (m_window != nullptr)
        glfwSetWindowTitle(m_window, label.c_str());

    return *this;
}

//TODO: find a way to invoke it indirectly? Also need to remake anyway.
GlfwWindow& GlfwWindow::setVSyncInterval(int interval)
{
    MakeContextCurrent();
    glfwSwapInterval(interval);

    return *this;
}

GlfwWindow& GlfwWindow::setWindowCloseFlag(bool flag)
{
    MakeContextCurrent();
    glfwSetWindowShouldClose(m_window, flag ? GLFW_TRUE : GLFW_FALSE);

    //When flag is setted manually there is no callback, so we should call it manually
    if (flag)
        OnClosing();

    return *this;
}

GlfwWindow& GlfwWindow::setWindowSize(int width, int height)
{
    m_windowSize = glm::ivec2(width, height);

    if (m_window != nullptr)
        glfwSetWindowSize(m_window, m_windowSize.x, m_windowSize.y);

    return *this;
}

void GlfwWindow::SetupCallbacks()
{

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
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

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto wnd = FromNativeWindow(window);
        if (action == GLFW_PRESS)
            wnd->OnMouseDown(button);
        else if (action == GLFW_RELEASE)
            wnd->OnMouseUp(button);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
        FromNativeWindow(window)->OnMouseMove(glm::vec2(x, y));
    });
    
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int w, int h) {
        FromNativeWindow(window)->OnResize(glm::ivec2(w, h));
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow *window) {
        FromNativeWindow(window)->OnClosing();
    });

    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow *window) {
        auto wnd = FromNativeWindow(window);
        wnd->OnWindowRefreshing();
        wnd->Render();
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        FromNativeWindow(window)->OnMouseScroll(glm::vec2(xoffset, yoffset));
    });
    
}

void GlfwWindow::MakeContextCurrent()
{
    GLFWwindow* actualContext = glfwGetCurrentContext();

    if (actualContext != m_window)
    {
        glfwMakeContextCurrent(m_window);

        //in common case gl functions must be updated after context switching, but fortunately in case "one context per one thread" it don't occured
        TryUpdateGlBindings();
    }
}