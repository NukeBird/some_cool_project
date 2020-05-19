#ifndef GLFW_WINDOW_HEADER
#define GLFW_WINDOW_HEADER

#include <iostream>
#include <functional>

#include <globjects/globjects.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>


struct GlfwException : public std::runtime_error
{
    GlfwException(const char* reason) : std::runtime_error(reason)
    {
    }
};

class MousePos
{
public:
    MousePos() = default;
    MousePos(const glm::vec2& pos, const glm::vec2& previousPos) : position(pos), previous_position(previousPos) {}
    MousePos(const MousePos&) = default;
    MousePos& operator=(const MousePos&) = default;

    const glm::vec2& getPos() const noexcept { return position; }
    glm::vec2 getDeltaPos() const noexcept { return position - previous_position; }
    const glm::vec2& getPreviousPos() const noexcept { return previous_position; }

private:
    glm::vec2 position, previous_position;
};

enum class GlfwCursorMode
{
    Normal = GLFW_CURSOR_NORMAL,
    Hidden = GLFW_CURSOR_HIDDEN,
    Disabled = GLFW_CURSOR_DISABLED
};

class GlfwWindow
{
public:
    GlfwWindow() = default;
    virtual ~GlfwWindow();

    void Run();

    bool isKeyDown(int keyCode) const { return window_impl && (glfwGetKey(window_impl, keyCode) == GLFW_PRESS); }
    bool isMouseKeyDown(int button) const { return window_impl && (glfwGetMouseButton(window_impl, button) == GLFW_PRESS); }

    GlfwWindow& setCursorMode(GlfwCursorMode cursorMode)
    {
        if (window_impl)
            glfwSetInputMode(window_impl, GLFW_CURSOR, static_cast<int>(cursorMode));

        return *this;
    }

    //TODO: probably all this functions should be made thread-safe or made available only from callbacks(for example moved to some helper class)
    GlfwWindow& setLabel(const std::string& label);
    const std::string& getLabel() const { return window_label; }

    GlfwWindow& setSize(int width, int height);
    const glm::ivec2& getSize() const { return window_size; }

    GlfwWindow& setVSyncInterval(int interval);
    GlfwWindow& setCloseFlag(bool flag);
    GlfwWindow& setNumMsaaSamples(int samples = 0);

    //event callbacks
    std::function<void(void)> InitializeCallback;
    std::function<void(double)> UpdateCallback;
    std::function<void(double)> RenderCallback;
    std::function<void(void)> RefreshingCallback;

    std::function<void(int)> KeyDownCallback;
    std::function<void(int)> KeyUpCallback;
    std::function<void(int)> KeyRepeatCallback;
    std::function<void(const glm::ivec2&)> ResizeCallback;
    std::function<void(void)> ClosingCallback;

    std::function<void(const MousePos&)> MouseMoveCallback;
    std::function<void(int)> MouseDownCallback;
    std::function<void(int)> MouseUpCallback;
    std::function<void(glm::vec2)> MouseScrollCallback;

protected:
    void OnInitialize() const { if (InitializeCallback) InitializeCallback(); }
    void OnUpdate(double deltaTime) const { if (UpdateCallback) UpdateCallback(deltaTime); }
    void OnRender(double deltaTime) const { if (RenderCallback) RenderCallback (deltaTime); }
    void OnWindowRefreshing() const { if (RefreshingCallback) RefreshingCallback(); }

    void OnKeyDown(int key) const
    {
        if (KeyDownCallback)
            KeyDownCallback(key);
    }

    void OnKeyUp(int key) const
    {
        if (KeyUpCallback)
            KeyUpCallback(key);
    }
    void OnKeyRepeat(int key) const { if (KeyRepeatCallback) KeyRepeatCallback(key); }
    
    void OnResize(const glm::ivec2& newSize) const 
    { 
        if (ResizeCallback) ResizeCallback(newSize); 

        window_size = newSize;
    }
    void OnClosing() const { if (ClosingCallback) ClosingCallback(); }
    
    void OnMouseMove(glm::vec2& mousePosition) const 
    {
        mousePosition.y = window_size.y - mousePosition.y; //WTF?

        if (MouseMoveCallback != nullptr) 
            MouseMoveCallback(MousePos{ mousePosition, previous_mouse_pos });

        previous_mouse_pos = mousePosition;
    }
    void OnMouseDown(int button) const { if (MouseDownCallback) MouseDownCallback(button);}
    void OnMouseUp(int button) const { if (MouseUpCallback) MouseUpCallback(button); }
    void OnMouseScroll(const glm::vec2& scrollDirection) const { if (MouseScrollCallback) MouseScrollCallback(scrollDirection); }

private:
    void SetupCallbacks();
    void MakeContextCurrent();

    void UpdateAndRender();

    static GlfwWindow* FromNativeWindow(const GLFWwindow* window);
    static void TryUpdateGlBindings();

private:
    const int OpenGL_Context_Version_Major = 4;
    const int OpenGL_Context_Version_Minor = 3;

private:
    GLFWwindow* window_impl = nullptr;

    std::string window_label = "New window";
    int num_msaa_samples = 0;

    //mutables is just for track some parameters in callbacks
    mutable glm::ivec2 window_size = glm::ivec2(800, 600);
    mutable glm::vec2 previous_mouse_pos;

    double previous_render_time = 0.0;
};

#endif
