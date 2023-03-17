#include "VulkanBuffer.h"

#include "DebugUtils.h"
#include "Device.h"

namespace VKRT {

VulkanBuffer* VulkanBuffer::Create(
    Device* device,
    const vk::DeviceSize& size,
    const vk::BufferUsageFlags& usageFlags,
    const vk::MemoryPropertyFlags& memoryFlags) {
    const vk::Device& logicalDevice = device->GetLogicalDevice();
    const vk::BufferCreateInfo bufferCreateInfo =
        vk::BufferCreateInfo().setSize(size).setUsage(usageFlags).setSharingMode(vk::SharingMode::eExclusive);
    const vk::Buffer bufferHandle = VKRT_ASSERT_VK(logicalDevice.createBuffer(bufferCreateInfo));

    const vk::MemoryRequirements memoryRequirements = logicalDevice.getBufferMemoryRequirements(bufferHandle);
    const vk::DeviceMemory memoryHandle = device->AllocateMemory(memoryFlags, memoryRequirements);

    VKRT_ASSERT_VK(logicalDevice.bindBufferMemory(bufferHandle, memoryHandle, 0));

    const vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo().setBuffer(bufferHandle).setOffset(0).setRange(size);

    return new VulkanBuffer(device, size, bufferHandle, memoryHandle, bufferInfo);
}

VulkanBuffer::VulkanBuffer(
    Device* device,
    vk::DeviceSize size,
    vk::Buffer bufferHandle,
    vk::DeviceMemory memoryHandle,
    vk::DescriptorBufferInfo descriptorInfo)
    : mDevice(device), mSize(size), mBufferHandle(bufferHandle), mMemoryHandle(memoryHandle), mDescriptorInfo(descriptorInfo) {
    mDevice->AddRef();
}

uint8_t* VulkanBuffer::MapBuffer() {
    const vk::Device& logicalDevice = mDevice->GetLogicalDevice();
    return static_cast<uint8_t*>(VKRT_ASSERT_VK(logicalDevice.mapMemory(mMemoryHandle, 0, mSize)));
}

void VulkanBuffer::UnmapBuffer() {
    const vk::Device& logicalDevice = mDevice->GetLogicalDevice();
    logicalDevice.unmapMemory(mMemoryHandle);
}

VulkanBuffer::~VulkanBuffer() {
    vk::Device& logicalDevice = mDevice->GetLogicalDevice();
    logicalDevice.destroyBuffer(mBufferHandle);
    logicalDevice.freeMemory(mMemoryHandle);
    mDevice->Release();
}

}  // namespace VKRT