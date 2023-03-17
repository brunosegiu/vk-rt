#include "Device.h"

#include <array>
#include <limits>

#include "DebugUtils.h"
#include "Instance.h"
#include "ResourceLoader.h"

namespace VKRT {

ResultValue<Device*> Device::Create() {
    const auto instanceResult = VulkanInstance::Create();
    if (instanceResult.result == Result::Success) {
        const auto instance = instanceResult.value;
        return VulkanInstance::CreateDevice(instance);
    }
    return {instanceResult.result, nullptr};
}

Device::Device(const std::shared_ptr<VulkanInstance>& instance, vk::PhysicalDevice physicalDevice)
    : mVulkanInstance(instance), mPhysicalDevice(physicalDevice) {
    const std::vector<vk::QueueFamilyProperties> queueFamiliesProperties = mPhysicalDevice.getQueueFamilyProperties();
    uint32_t queueFamilyIndex = 0;
    for (const auto& properties : queueFamiliesProperties) {
        if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
            break;
        }
        queueFamilyIndex += 1;
    }
    VKRT_ASSERT(queueFamilyIndex < static_cast<uint32_t>(queueFamiliesProperties.size()));
    const std::vector<float> queuePriorities{1.0f};
    vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo().setQueueFamilyIndex(queueFamilyIndex).setQueuePriorities(queuePriorities);

    const vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo().setQueueCreateInfos(queueCreateInfo);
    mLogicalDevice = VKRT_ASSERT_VK(mPhysicalDevice.createDevice(deviceCreateInfo));

    mComputeQueue = mLogicalDevice.getQueue(queueFamilyIndex, 0);

    const vk::CommandPoolCreateInfo commandPoolCreateInfo =
        vk::CommandPoolCreateInfo().setQueueFamilyIndex(queueFamilyIndex).setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    mCommandPool = VKRT_ASSERT_VK(mLogicalDevice.createCommandPool(commandPoolCreateInfo));
}

vk::DeviceMemory Device::AllocateMemory(const vk::MemoryPropertyFlags& memoryFlags, const vk::MemoryRequirements memoryRequirements) {
    // Find suitable memory to allocate the buffer in
    const vk::PhysicalDeviceMemoryProperties memoryProperties = mPhysicalDevice.getMemoryProperties();
    uint32_t selectedMemoryIndex = 0;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryProperties.memoryTypeCount; ++memoryIndex) {
        if (memoryRequirements.memoryTypeBits & (1 << memoryIndex) && (memoryProperties.memoryTypes[memoryIndex].propertyFlags & memoryFlags)) {
            selectedMemoryIndex = memoryIndex;
            break;
        }
    }

    // Allocate memory for the buffer
    const vk::MemoryAllocateInfo allocateInfo =
        vk::MemoryAllocateInfo().setAllocationSize(memoryRequirements.size).setMemoryTypeIndex(selectedMemoryIndex);
    return VKRT_ASSERT_VK(mLogicalDevice.allocateMemory(allocateInfo));
}

VulkanBuffer* Device::CreateBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usageFlags, const vk::MemoryPropertyFlags& memoryFlags) {
    return VulkanBuffer::Create(this, size, usageFlags, memoryFlags);
}

vk::CommandBuffer Device::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo commandInfo =
        vk::CommandBufferAllocateInfo().setCommandBufferCount(1).setCommandPool(mCommandPool).setLevel(vk::CommandBufferLevel::ePrimary);
    return VKRT_ASSERT_VK(mLogicalDevice.allocateCommandBuffers(commandInfo))[0];
}

void Device::SubmitCommand(const vk::CommandBuffer& commandBuffer, const vk::Fence& fence) {
    const vk::SubmitInfo submitInfo = vk::SubmitInfo().setCommandBuffers(commandBuffer);
    VKRT_ASSERT_VK(mComputeQueue.submit(submitInfo, fence));
}

void Device::DestroyCommand(vk::CommandBuffer& commandBuffer) {
    mLogicalDevice.freeCommandBuffers(mCommandPool, commandBuffer);
}

vk::Fence Device::CreateFence() {
    const vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo();
    return VKRT_ASSERT_VK(mLogicalDevice.createFence(fenceInfo));
}

void Device::WaitForFence(vk::Fence& fence) {
    VKRT_ASSERT_VK(mLogicalDevice.waitForFences(fence, true, (std::numeric_limits<uint64_t>::max)()));
}

void Device::DestroyFence(vk::Fence& fence) {
    mLogicalDevice.destroyFence(fence);
}

Device::~Device() {
    mLogicalDevice.destroyCommandPool(mCommandPool);
    mLogicalDevice.destroy();
    mVulkanInstance = nullptr;
}

}  // namespace VKRT
