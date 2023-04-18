#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

inline void panic(const char *msg)
{
    std::fprintf(stderr, "%s", msg);
    std::abort();
}

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
        // create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
            glfwCreateWindow(width, height, "demo", nullptr, nullptr), glfwDestroyWindow);

        // create vulkan instance
        const std::vector<const char *> requiredExtensions = [] {
            uint32_t count = 0;
            const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&count);
            return std::vector<const char *>(requiredExtensions, requiredExtensions + count);
        }();

        const std::vector<const char *> requiredLayers = {"VK_LAYER_KHRONOS_validation"};

        const VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                        .pApplicationName = "demo",
                                        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                        .pEngineName = "demo",
                                        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                                        .apiVersion = VK_API_VERSION_1_0};

        const VkInstanceCreateInfo instanceCreateInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                                      .pApplicationInfo = &appInfo,
                                                      .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                                                      .ppEnabledLayerNames = requiredLayers.data(),
                                                      .enabledExtensionCount =
                                                          static_cast<uint32_t>(requiredExtensions.size()),
                                                      .ppEnabledExtensionNames = requiredExtensions.data()};

        VkInstance instance = VK_NULL_HANDLE;
        auto rv = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
        if (rv != VK_SUCCESS)
            panic("Failed to create instance: %d\n", rv);

        std::printf("Vulkan instance: %p\n", instance);

        // create surface
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        rv = glfwCreateWindowSurface(instance, window.get(), nullptr, &surface);
        if (rv != VK_SUCCESS)
            panic("Failed cto create surface: %d\n", rv);

        std::printf("Vulkan surface: %p\n", surface);

        // find a physical device
        VkPhysicalDevice physicalDevice = [instance]() -> VkPhysicalDevice {
            uint32_t count = 0;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            if (count == 0)
                return VK_NULL_HANDLE;
            std::vector<VkPhysicalDevice> devices(count);
            vkEnumeratePhysicalDevices(instance, &count, devices.data());

            // find the first physical device that supports the swapchain extension
            auto it = std::find_if(devices.begin(), devices.end(), [](const VkPhysicalDevice device) {
                uint32_t count;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
                std::vector<VkExtensionProperties> extensionProperties(count);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensionProperties.data());
                const auto supported =
                    std::any_of(extensionProperties.begin(), extensionProperties.end(),
                                [](const VkExtensionProperties &properties) {
                                    return strcmp(properties.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
                                });
                return supported;
            });

            return it != devices.end() ? *it : VK_NULL_HANDLE;
        }();
        if (physicalDevice == VK_NULL_HANDLE)
            panic("Failed to find a physical device\n");

        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            std::printf("Physical device: %s\n", properties.deviceName);
        }

        // find a queue family that supports graphics
        int queueFamilyIndex = [physicalDevice]() -> int {
            uint32_t count;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
            if (count == 0)
                return -1;
            std::vector<VkQueueFamilyProperties> properties(count);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, properties.data());
            auto it = std::find_if(properties.begin(), properties.end(), [](const VkQueueFamilyProperties &properties) {
                return properties.queueCount > 0 && (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT);
            });

            return it != properties.end() ? std::distance(properties.begin(), it) : -1;
        }();
        if (queueFamilyIndex == -1)
            panic("Can't find a queue family with required properties");

        std::printf("Queue family index: %d\n", queueFamilyIndex);

        // create the device
        const float queuePriority = 1.0f;

        const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                               .queueFamilyIndex =
                                                                   static_cast<uint32_t>(queueFamilyIndex),
                                                               .queueCount = 1,
                                                               .pQueuePriorities = &queuePriority};

        static const std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        const VkDeviceCreateInfo deviceCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                                     .queueCreateInfoCount = 1,
                                                     .pQueueCreateInfos = &deviceQueueCreateInfo,
                                                     .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                                                     .ppEnabledExtensionNames = extensions.data()};

        VkDevice device = VK_NULL_HANDLE;
        rv = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
        if (rv != VK_SUCCESS)
            panic("Failed to create device");

        std::printf("Device: %p\n", device);

        while (!glfwWindowShouldClose(window.get()))
        {
            // do something
            glfwPollEvents();
        }

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    glfwTerminate();
}
