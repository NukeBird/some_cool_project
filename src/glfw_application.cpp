#include "glfw_application.h"

//#include <GLFW/glfw3.h>


GlfwApplication& GlfwApplication::get()
{
    static GlfwApplication keeper;
    return keeper;
}


std::shared_ptr<GlfwWindow> GlfwApplication::createWindow()
{
    auto pWindow = new GlfwWindow({});
    std::shared_ptr<GlfwWindow> window { pWindow };

    std::lock_guard lock { windows_registry_mutex };
    windows_registry.push_back(window);

    return window;
}

void GlfwApplication::run()
{
    while (!exit_triggered)
    {
        {
            std::lock_guard lock{ windows_registry_mutex };

            windows_registry.erase(std::remove_if(std::begin(windows_registry), std::end(windows_registry), 
           [](std::shared_ptr<GlfwWindow>& window)
                {
                    if (window->getCloseFlag() || window.use_count() == 1)
                    {
                        window->Close();
                        return true; //
                    }

                    window->UpdateAndRender();
                    return false;
                }
            ), std::end(windows_registry));

        }

        if (windows_registry.empty() && OnNoActiveWindows)
            OnNoActiveWindows();

        glfwPollEvents();
    }
}

GlfwApplication::GlfwApplication()
{
    if (!glfwInit())
        throw GlfwException("Initialization failed");

    glfwSetErrorCallback([](int, const char* message)
    {
        throw GlfwException(message);
    });
}

GlfwApplication::~GlfwApplication()
{
    glfwTerminate();
}
