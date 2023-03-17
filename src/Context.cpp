#include "Context.h"

#include <GLFW/glfw3.h>

namespace VKRT {

ResultValue<Context*> Context::Create(Window* window) {
    auto [instanceResult, instance] = Instance::Create(window);
    if (instanceResult == Result::Success) {
        auto [deviceResult, device] = Device::Create(instance);
        if (deviceResult == Result::Success) {
            return {Result::Success, new Context(window, instance, device)};
        } else {
            instance->Release();
        }
    }
}

Context::Context(Window* window, Instance* instance, Device* device) {
    window->AddRef();
    mWindow = window;
    mInstance = instance;
    mDevice = device;
}

Context::~Context() {
    mDevice->Release();
    mInstance->Release();
    mWindow->Release();
}

}  // namespace VKRT