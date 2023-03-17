#pragma once

#include <memory>

#include "Result.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"
#include "Window.h"

namespace VKRT {

class VulkanInstance;

class Device : public RefCountPtr {
public:
    static ResultValue<Device*> Create(Window* window);

    Device(const std::shared_ptr<VulkanInstance>& instance, vk::PhysicalDevice physicalDevice);

    // Memory handling
    vk::DeviceMemory AllocateMemory(const vk::MemoryPropertyFlags& memoryFlags, const vk::MemoryRequirements memoryRequirements);

    VulkanBuffer* CreateBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usageFlags, const vk::MemoryPropertyFlags& memoryFlags);

    vk::CommandBuffer CreateCommandBuffer();
    void SubmitCommand(const vk::CommandBuffer& commandBuffer, const vk::Fence& fence);
    void DestroyCommand(vk::CommandBuffer& commandBuffer);

    vk::Fence CreateFence();
    void WaitForFence(vk::Fence& fence);
    void DestroyFence(vk::Fence& fence);

    vk::Device& GetLogicalDevice() { return mLogicalDevice; }

    ~Device();

private:
    std::shared_ptr<VulkanInstance> mVulkanInstance;
    vk::PhysicalDevice mPhysicalDevice;
    vk::Device mLogicalDevice;
    vk::Queue mComputeQueue;
    vk::CommandPool mCommandPool;
};

}  // namespace VKRT
