#pragma once

#include <string>
#include <vector>

#include "Device.h"
#include "Instance.h"
#include "RefCountPtr.h"
#include "Result.h"
#include "Swapchain.h"
#include "VulkanBase.h"
#include "Window.h"

namespace VKRT {
class Context : public RefCountPtr {
public:
    Context(
        ScopedRefPtr<Window> window,
        ScopedRefPtr<Instance> instance,
        vk::SurfaceKHR surface,
        ScopedRefPtr<Device> device);

    ScopedRefPtr<Window> GetWindow() { return mWindow; }
    ScopedRefPtr<Instance> GetInstance() { return mInstance; }
    const vk::SurfaceKHR& GetSurface() { return mSurface; }
    ScopedRefPtr<Device> GetDevice() { return mDevice; }
    ScopedRefPtr<Swapchain> GetSwapchain() { return mSwapchain; }

    void Destroy();

    ~Context();

private:
    ScopedRefPtr<Window> mWindow;
    vk::SurfaceKHR mSurface;
    ScopedRefPtr<Instance> mInstance;
    ScopedRefPtr<Device> mDevice;
    ScopedRefPtr<Swapchain> mSwapchain;
};

}  // namespace VKRT
