#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>


#include "glfw_window.h"

class GlfwException : public std::runtime_error
{
public:
    GlfwException(const char* reason) : std::runtime_error(reason)
    {
    }
};


class GlfwApplication
{
public:
    static GlfwApplication& get();

    GlfwApplication(const GlfwApplication&) = delete;
    GlfwApplication(GlfwApplication&&) = delete;
    GlfwApplication& operator=(const GlfwApplication&) = delete;
    GlfwApplication& operator=(GlfwApplication&&) = delete;

    std::function<void()> OnNoActiveWindows{};

    //thread-safe
    std::shared_ptr<GlfwWindow> createWindow();

    //should be called once
    void run();
    void stop() { exit_triggered = true; }

private:
    GlfwApplication();
    ~GlfwApplication();

private:
    std::atomic<bool> exit_triggered = false;
    std::mutex windows_registry_mutex;
    std::vector<std::shared_ptr<GlfwWindow>> windows_registry;
};
