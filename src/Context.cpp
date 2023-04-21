#include "Context.h"

#include <GLFW/glfw3.h>

#include "DebugUtils.h"

namespace VKRT {

Context::Context(
    ScopedRefPtr<Window> window,
    ScopedRefPtr<Instance> instance,
    vk::SurfaceKHR surface,
    ScopedRefPtr<Device> device) {
    mWindow = window;
    mInstance = instance;
    mSurface = surface;
    mDevice = device;
    mDevice->SetContext(this);
    mSwapchain = new Swapchain(this);
}

void Context::Destroy() {
    VKRT_ASSERT_VK(mDevice->GetLogicalDevice().waitIdle());
    mSwapchain = nullptr;
    mInstance->DestroySurface(mSurface);
    mDevice = nullptr;
    mInstance = nullptr;
    mWindow = nullptr;
}

Context::~Context() {}

}  // namespace VKRT