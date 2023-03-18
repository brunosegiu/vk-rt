#include "Window.h"

#include <GLFW/glfw3.h>

#ifdef VKRT_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace VKRT {

ResultValue<Window*> Window::Create() {
    return {Result::Success, new Window()};
}

Window::Window() : mNativeHandle(nullptr) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    mNativeHandle = glfwCreateWindow(1280, 720, "VKRT", nullptr, nullptr);
}

void Window::ProcessEvents() {
    while (!glfwWindowShouldClose(mNativeHandle)) {
        glfwPollEvents();
    }
}

std::vector<std::string> Window::GetRequiredVulkanExtensions() {
    uint32_t requiredExtensionCount = 0;
    const char** requiredExtensionNames = nullptr;
    requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<std::string> result;
    result.reserve(requiredExtensionCount);
    for (int extIndex = 0; extIndex < requiredExtensionCount; ++extIndex) {
        result.push_back(requiredExtensionNames[extIndex]);
    }
    return result;
}

void* Window::GetWindowOSHandle() {
#ifdef VKRT_PLATFORM_WINDOWS
    return glfwGetWin32Window(mNativeHandle);
#endif
}

Window::Size2D Window::GetSize() {
    Size2D size;
    glfwGetFramebufferSize(mNativeHandle, reinterpret_cast<int*>(&size.width), reinterpret_cast<int*>(&size.height));
    return size;
}

Window::~Window() {
    if (mNativeHandle != nullptr) {
        glfwDestroyWindow(mNativeHandle);
    }
    glfwTerminate();
}
}  // namespace VKRT