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
    static ResultValue<Device*> Create(Instance* instance);

    Device(Instance* instance, vk::PhysicalDevice physicalDevice);

    void SetContext(Context* context);

    // Memory handling
    vk::DeviceMemory AllocateMemory(
        const vk::MemoryPropertyFlags& memoryFlags,
        const vk::MemoryRequirements memoryRequirements,
        const vk::MemoryAllocateFlags& memoryAllocateFlags);

    VulkanBuffer* CreateBuffer(
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

    struct SwapchainCapabilities {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<vk::SurfaceFormatKHR> supportedFormats;
        std::vector<vk::PresentModeKHR> supportedPresentModes;
    };
    SwapchainCapabilities GetSwapchainCapabilities(vk::SurfaceKHR surface);

    ~Device();

private:
    Context* mContext;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mLogicalDevice;
    vk::Queue mGraphicsQueue;
    vk::CommandPool mCommandPool;
};

}  // namespace VKRT
