#include "Window.h"

#include <GLFW/glfw3.h>

#ifdef VKRT_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include "Context.h"
#include "Device.h"
#include "InputManager.h"
#include "Instance.h"

namespace VKRT {

ResultValue<Window*> Window::Create() {
    return {Result::Success, new Window()};
}

Window::Window() : mNativeHandle(nullptr), mContext(nullptr) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    mNativeHandle = glfwCreateWindow(1280, 720, "VKRT", nullptr, nullptr);
    mInputManager = new InputManager(this);
}

bool Window::Update() {
    return mInputManager->Update();
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

ResultValue<Context*> Window::CreateContext() {
    if (mContext == nullptr) {
        auto [instanceResult, instance] = Instance::Create(this);
        if (instanceResult == Result::Success) {
            vk::SurfaceKHR surface = instance->CreateSurface(this);
            auto [deviceResult, device] = Device::Create(instance, surface);
            if (deviceResult == Result::Success) {
                mContext = new Context(this, instance, surface, device);
                mContext->AddRef();
                return {Result::Success, mContext};
            } else {
                instance->Release();
                return {deviceResult, nullptr};
            }
        }
        return {instanceResult, nullptr};
    }
}

Window::~Window() {
    if (mNativeHandle != nullptr) {
        glfwDestroyWindow(mNativeHandle);
    }
    if (mContext != nullptr) {
        mContext->Release();
    }
    glfwTerminate();
}
}  // namespace VKRT