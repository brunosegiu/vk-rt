#pragma once

#include <memory>

#include "Device.h"
#include "Result.h"
#include "VulkanBase.h"

namespace VKRT {
class VulkanInstance {
public:
    static ResultValue<std::shared_ptr<VulkanInstance>> Create();
    static ResultValue<Device*> CreateDevice(const std::shared_ptr<VulkanInstance>& instance);

    VulkanInstance(const vk::Instance& instance);

    ~VulkanInstance();

private:
    ResultValue<vk::PhysicalDevice> FindSuitablePhysicalDevice();

    vk::Instance mInstanceHandle;
    vk::DispatchLoaderDynamic mDynamicDispatcher;
#if defined(VKRT_DEBUG)
    vk::DebugUtilsMessengerEXT mDebugMessenger;
#endif
};
}  // namespace VKRT
