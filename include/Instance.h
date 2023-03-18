#pragma once

#include "Device.h"
#include "Result.h"
#include "VulkanBase.h"
#include "Window.h"

namespace VKRT {
class Instance : public RefCountPtr {
public:
    static ResultValue<Instance*> Create(Window* window);

    Instance(const vk::Instance& instance);

    ResultValue<vk::PhysicalDevice> FindSuitablePhysicalDevice();

    vk::SurfaceKHR CreateSurface(Window* window);
    void DestroySurface(vk::SurfaceKHR surface);

    ~Instance();

private:
    vk::Instance mInstanceHandle;
    vk::DispatchLoaderDynamic mDynamicDispatcher;
#if defined(VKRT_DEBUG)
    vk::DebugUtilsMessengerEXT mDebugMessenger;
#endif
};
}  // namespace VKRT
