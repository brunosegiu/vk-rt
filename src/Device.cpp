#include "Device.h"

#include <array>
#include <limits>

#include "DebugUtils.h"
#include "Instance.h"
#include "ResourceLoader.h"
#include "VulkanBuffer.h"
#include "Window.h"

namespace VKRT {

ResultValue<Device*> Device::Create(Instance* instance, const vk::SurfaceKHR& surface) {
    auto [result, physicalDevice] = instance->FindSuitablePhysicalDevice(surface);
    if (result == Result::Success) {
        return {Result::Success, new Device(instance, physicalDevice, surface)};
    }
    return {Result::InvalidDeviceError, nullptr};
}

Device::Device(Instance* instance, vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR& surface)
    : mContext(nullptr), mPhysicalDevice(physicalDevice) {
    const std::vector<vk::QueueFamilyProperties> queueFamiliesProperties = mPhysicalDevice.getQueueFamilyProperties();
    uint32_t queueFamilyIndex = 0;
    for (const auto& properties : queueFamiliesProperties) {
        if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (VKRT_ASSERT_VK(physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface))) {
                break;
            }
        }
        ++queueFamilyIndex;
    }
    VKRT_ASSERT(queueFamilyIndex < static_cast<uint32_t>(queueFamiliesProperties.size()));
    const std::vector<float> queuePriorities{1.0f};
    vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo().setQueueFamilyIndex(queueFamilyIndex).setQueuePriorities(queuePriorities);

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures =
        vk::PhysicalDeviceBufferDeviceAddressFeatures().setBufferDeviceAddress(true);

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures =
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR().setRayTracingPipeline(true).setPNext(&bufferDeviceAddressFeatures);

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures =
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR().setAccelerationStructure(true).setPNext(&rayTracingFeatures);

    const vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
                                                      .setQueueCreateInfos(queueCreateInfo)
                                                      .setPNext(&accelerationStructureFeatures)
                                                      .setPEnabledExtensionNames(Instance::sRequiredDeviceExtensions);
    mLogicalDevice = VKRT_ASSERT_VK(mPhysicalDevice.createDevice(deviceCreateInfo));

    mGraphicsQueue = mLogicalDevice.getQueue(queueFamilyIndex, 0);

    const vk::CommandPoolCreateInfo commandPoolCreateInfo =
        vk::CommandPoolCreateInfo().setQueueFamilyIndex(queueFamilyIndex).setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    mCommandPool = VKRT_ASSERT_VK(mLogicalDevice.createCommandPool(commandPoolCreateInfo));

    mDispatcher = vk::DispatchLoaderDynamic(instance->GetHandle(), vkGetInstanceProcAddr, mLogicalDevice, vkGetDeviceProcAddr);
}

void Device::SetContext(Context* context) {
    mContext = context;
    mContext->AddRef();
}

vk::DeviceMemory Device::AllocateMemory(
    const vk::MemoryPropertyFlags& memoryFlags,
    const vk::MemoryRequirements memoryRequirements,
    const vk::MemoryAllocateFlags& memoryAllocateFlags) {
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
    vk::MemoryAllocateFlagsInfo memoryAllocateFlagsInfo = vk::MemoryAllocateFlagsInfo().setFlags(memoryAllocateFlags);
    vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo().setAllocationSize(memoryRequirements.size).setMemoryTypeIndex(selectedMemoryIndex);
    if (memoryAllocateFlags != vk::MemoryAllocateFlags()) {
        allocateInfo.setPNext(&memoryAllocateFlagsInfo);
    }
    return VKRT_ASSERT_VK(mLogicalDevice.allocateMemory(allocateInfo));
}

VulkanBuffer* Device::CreateBuffer(
    const vk::DeviceSize& size,
    const vk::BufferUsageFlags& usageFlags,
    const vk::MemoryPropertyFlags& memoryFlags,
    const vk::MemoryAllocateFlags& memoryAllocateFlags) {
    VKRT_ASSERT(mContext != nullptr);
    return VulkanBuffer::Create(mContext, size, usageFlags, memoryFlags, memoryAllocateFlags);
}

vk::CommandBuffer Device::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo commandInfo =
        vk::CommandBufferAllocateInfo().setCommandBufferCount(1).setCommandPool(mCommandPool).setLevel(vk::CommandBufferLevel::ePrimary);
    return VKRT_ASSERT_VK(mLogicalDevice.allocateCommandBuffers(commandInfo))[0];
}

void Device::SubmitCommand(const vk::CommandBuffer& commandBuffer, const vk::Fence& fence) {
    const vk::SubmitInfo submitInfo = vk::SubmitInfo().setCommandBuffers(commandBuffer);
    VKRT_ASSERT_VK(mGraphicsQueue.submit(submitInfo, fence));
}

void Device::SubmitCommandAndFlush(const vk::CommandBuffer& commandBuffer) {
    vk::Fence fence = CreateFence();
    SubmitCommand(commandBuffer, fence);
    WaitForFence(fence);
    DestroyFence(fence);
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

Device::SwapchainCapabilities Device::GetSwapchainCapabilities(vk::SurfaceKHR surface) {
    SwapchainCapabilities capabilities{
        .surfaceCapabilities = VKRT_ASSERT_VK(mPhysicalDevice.getSurfaceCapabilitiesKHR(surface)),
        .supportedFormats = VKRT_ASSERT_VK(mPhysicalDevice.getSurfaceFormatsKHR(surface)),
        .supportedPresentModes = VKRT_ASSERT_VK(mPhysicalDevice.getSurfacePresentModesKHR(surface)),
    };
    return capabilities;
}

vk::PhysicalDeviceRayTracingPipelinePropertiesKHR Device::GetRayTracingProperties() {
    auto result = mPhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    return result.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
}

Device::~Device() {
    mLogicalDevice.waitIdle();
    mLogicalDevice.destroyCommandPool(mCommandPool);
    mLogicalDevice.destroy();
    if (mContext != nullptr) {
        mContext->Release();
    }
}

}  // namespace VKRT
