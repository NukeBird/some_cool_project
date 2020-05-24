#pragma once

#include <globjects/globjects.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>


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

enum class GlfwOpenglProfile
{
    Any = GLFW_OPENGL_ANY_PROFILE,
    Core = GLFW_OPENGL_CORE_PROFILE,
    Compat = GLFW_OPENGL_COMPAT_PROFILE
};

struct GlfwContextParameters
{
    GlfwOpenglProfile profile = GlfwOpenglProfile::Core;
    int context_major_version = 4;
    int context_minor_version = 3;
    int msaa_samples = 0;
};


class GlfwWindow final
{
friend class GlfwApplication;

public:
    ///@thread_safety main thread
    bool isKeyDown(int keyCode) const;
    ///@thread_safety main thread
    bool isMouseKeyDown(int button) const;

    ///@thread_safety main thread
    GlfwWindow& setCursorMode(GlfwCursorMode cursorMode);

    ///@thread_safety main thread
    GlfwWindow& setLabel(const std::string& label);
    const std::string& getLabel() const { return window_label; }

    ///@thread_safety main thread
    GlfwWindow& setSize(int width, int height);
    const glm::ivec2& getSize() const { return window_size; }

    ///@thread_safety main thread
    void requestAttention();

    ///@thread_safety any thread
    GlfwWindow& setVSyncInterval(int interval);

    ///@thread_safety any thread
    GlfwWindow& setCloseFlag(bool flag);
    bool getCloseFlag() const;


    //event callbacks
    std::function<void(void)> InitializeCallback {};
    std::function<void(double)> UpdateCallback {};
    std::function<void(double)> RenderCallback {};
    std::function<void(void)> RefreshingCallback {};

    std::function<void(int)> KeyDownCallback {};
    std::function<void(int)> KeyUpCallback {};
    std::function<void(int)> KeyRepeatCallback {};
    std::function<void(const glm::ivec2&)> ResizeCallback {};
    std::function<void(void)> ClosingCallback {};

    std::function<void(const MousePos&)> MouseMoveCallback {};
    std::function<void(int)> MouseDownCallback {};
    std::function<void(int)> MouseUpCallback {};
    std::function<void(glm::vec2)> MouseScrollCallback {};

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
    
    void OnResize(glm::ivec2 newSize) const 
    { 
        if (ResizeCallback) ResizeCallback(newSize); 

        window_size = newSize;
    }
    void OnClosing() const { if (ClosingCallback) ClosingCallback(); }
    
    void OnMouseMove(glm::vec2 mousePosition) const 
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
    ///@thread_safety main thread
    GlfwWindow(GlfwContextParameters params);

    void SetupCallbacks();
    void MakeContextCurrent();

    ///@thread_safety main thread
    void Close();
    ///@thread_safety any thread
    void UpdateAndRender();

    static GlfwWindow* FromNativeWindow(const GLFWwindow* window);
    static void TryUpdateGlBindings();


private:
    const GlfwContextParameters context_parameters;
    GLFWwindow* window_impl { nullptr };

    std::string window_label{ "New window"};

    bool isInitialized{ false };

    //mutables is just for track some parameters in callbacks
    mutable glm::ivec2 window_size { 800, 600 };
    mutable glm::vec2 previous_mouse_pos {};

    double previous_render_time {0.0};
};
