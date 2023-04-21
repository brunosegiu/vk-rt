#pragma once

#include <memory>

#include "RefCountPtr.h"
#include "Result.h"
#include "VulkanBase.h"

namespace VKRT {

class Instance;
class VulkanBuffer;
class Context;

class Device : public RefCountPtr {
public:
    static ResultValue<ScopedRefPtr<Device>> Create(
        ScopedRefPtr<Instance> instance,
        const vk::SurfaceKHR& surface);

    Device(
        ScopedRefPtr<Instance> instance,
        vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR& surface);

    void SetContext(ScopedRefPtr<Context> context);

    // Memory handling
    vk::DeviceMemory AllocateMemory(
        const vk::MemoryPropertyFlags& memoryFlags,
        const vk::MemoryRequirements memoryRequirements,
        const vk::MemoryAllocateFlags& memoryAllocateFlags = {});

    ScopedRefPtr<VulkanBuffer> CreateBuffer(
        const vk::DeviceSize& size,
        const vk::BufferUsageFlags& usageFlags,
        const vk::MemoryPropertyFlags& memoryFlags,
        const vk::MemoryAllocateFlags& memoryAllocateFlags = {});

    vk::CommandBuffer CreateCommandBuffer();
    void SubmitCommand(const vk::CommandBuffer& commandBuffer, const vk::Fence& fence);
    void SubmitCommandAndFlush(const vk::CommandBuffer& commandBuffer);
    void DestroyCommand(vk::CommandBuffer& commandBuffer);

    vk::Fence CreateFence();
    void WaitForFence(vk::Fence& fence);
    void DestroyFence(vk::Fence& fence);

    vk::Device& GetLogicalDevice() { return mLogicalDevice; }
    vk::DispatchLoaderDynamic& GetDispatcher() { return mDispatcher; }
    const vk::Queue& GetQueue() { return mGraphicsQueue; }

    struct SwapchainCapabilities {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<vk::SurfaceFormatKHR> supportedFormats;
        std::vector<vk::PresentModeKHR> supportedPresentModes;
    };
    SwapchainCapabilities GetSwapchainCapabilities(vk::SurfaceKHR surface);

    vk::PhysicalDeviceProperties GetDeviceProperties();
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingProperties();

    ~Device();

private:
    ScopedRefPtr<Context> mContext;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mLogicalDevice;
    vk::Queue mGraphicsQueue;
    vk::CommandPool mCommandPool;
    vk::DispatchLoaderDynamic mDispatcher;
};

}  // namespace VKRT
