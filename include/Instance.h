#pragma once

#include "Device.h"
#include "Result.h"
#include "VulkanBase.h"
#include "Window.h"

namespace VKRT {
class Instance : public RefCountPtr {
public:
    static ResultValue<ScopedRefPtr<Instance>> Create(ScopedRefPtr<Window> window);

    Instance(const vk::Instance& instance);

    ResultValue<vk::PhysicalDevice> FindSuitablePhysicalDevice(const vk::SurfaceKHR& surface);

    vk::SurfaceKHR CreateSurface(ScopedRefPtr<Window> window);
    void DestroySurface(vk::SurfaceKHR surface);

    vk::Instance& GetHandle() { return mInstanceHandle; }

    static const std::vector<const char*> sRequiredDeviceExtensions;

    ~Instance();

private:
    vk::Instance mInstanceHandle;
    vk::DispatchLoaderDynamic mDynamicDispatcher;
#if defined(VKRT_ENABLE_VALIDATION)
    vk::DebugUtilsMessengerEXT mDebugMessenger;
#endif
};
}  // namespace VKRT
