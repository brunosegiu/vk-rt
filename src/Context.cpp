#include "Context.h"

#include <GLFW/glfw3.h>

namespace VKRT {

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