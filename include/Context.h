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
    static ResultValue<Context*> Create(Window* window);

    Context(Window* window, Instance* instance, Device* device);

    Window* GetWindow() { return mWindow; }
    Instance* GetInstance() { return mInstance; }
    Device* GetDevice() { return mDevice; }

    ~Context();

private:
    Window* mWindow;
    Instance* mInstance;
    Device* mDevice;
    Swapchain* mSwapchain;
};

}  // namespace VKRT
