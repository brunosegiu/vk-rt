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
    Context(Window* window, Instance* instance, vk::SurfaceKHR surface, Device* device);

    Window* GetWindow() { return mWindow; }
    Instance* GetInstance() { return mInstance; }
    const vk::SurfaceKHR& GetSurface() { return mSurface; }
    Device* GetDevice() { return mDevice; }
    Swapchain* GetSwapchain() { return mSwapchain; }

    ~Context();

private:
    Window* mWindow;
    vk::SurfaceKHR mSurface;
    Instance* mInstance;
    Device* mDevice;
    Swapchain* mSwapchain;
};

}  // namespace VKRT
