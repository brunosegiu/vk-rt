#include "Context.h"

#include <GLFW/glfw3.h>

namespace VKRT {

ResultValue<Context*> Context::Create(Window* window) {
    auto [instanceResult, instance] = Instance::Create(window);
    if (instanceResult == Result::Success) {
        vk::SurfaceKHR surface = instance->CreateSurface(window);
        auto [deviceResult, device] = Device::Create(instance, surface);
        if (deviceResult == Result::Success) {
            return {Result::Success, new Context(window, instance, surface, device)};
        } else {
            instance->Release();
            return {deviceResult, nullptr};
        }
    }
    return {instanceResult, nullptr};
}

Context::Context(Window* window, Instance* instance, vk::SurfaceKHR surface, Device* device) {
    window->AddRef();
    mWindow = window;
    mInstance = instance;
    mSurface = surface;
    mDevice = device;
    mDevice->SetContext(this);
    mSwapchain = new Swapchain(this);
}

Context::~Context() {
    mDevice->Release();
    mInstance->DestroySurface(mSurface);
    mInstance->Release();
    mWindow->Release();
}

}  // namespace VKRT