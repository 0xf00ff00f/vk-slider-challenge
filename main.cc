#include <cstdio>
#include <cstdlib>

#include <memory>

#include <GLFW/glfw3.h>

template<typename... Args>
inline void panic(const char *fmt, const Args &...args)
{
    std::fprintf(stderr, fmt, args...);
    std::abort();
}

int main()
{
    glfwInit();
    glfwSetErrorCallback([](int error, const char *description) { panic("GLFW error %d: %s\n", error, description); });

    const auto width = 800;
    const auto height = 400;

    {
        std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
            glfwCreateWindow(width, height, "demo", nullptr, nullptr), glfwDestroyWindow);

        while (!glfwWindowShouldClose(window.get()))
        {
            // do something
            glfwPollEvents();
        }
    }

    glfwTerminate();
}
