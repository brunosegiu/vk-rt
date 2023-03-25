#include "VulkanBuffer.h"

#include "DebugUtils.h"
#include "Device.h"

namespace VKRT {

VulkanBuffer* VulkanBuffer::Create(
    Context* context,
    const vk::DeviceSize& size,
    const vk::BufferUsageFlags& usageFlags,
    const vk::MemoryPropertyFlags& memoryFlags,
    const vk::MemoryAllocateFlags& memoryAllocateFlags) {
    const vk::Device& logicalDevice = context->GetDevice()->GetLogicalDevice();
    const vk::BufferCreateInfo bufferCreateInfo =
        vk::BufferCreateInfo().setSize(size).setUsage(usageFlags).setSharingMode(vk::SharingMode::eExclusive);
    const vk::Buffer bufferHandle = VKRT_ASSERT_VK(logicalDevice.createBuffer(bufferCreateInfo));

    const vk::MemoryRequirements memoryRequirements = logicalDevice.getBufferMemoryRequirements(bufferHandle);
    const vk::DeviceMemory memoryHandle = context->GetDevice()->AllocateMemory(memoryFlags, memoryRequirements, memoryAllocateFlags);

    VKRT_ASSERT_VK(logicalDevice.bindBufferMemory(bufferHandle, memoryHandle, 0));

    const vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo().setBuffer(bufferHandle).setOffset(0).setRange(size);

    return new VulkanBuffer(context, size, bufferHandle, memoryHandle, bufferInfo);
}

VulkanBuffer::VulkanBuffer(
    Context* context,
    vk::DeviceSize size,
    vk::Buffer bufferHandle,
    vk::DeviceMemory memoryHandle,
    vk::DescriptorBufferInfo descriptorInfo)
    : mContext(context), mSize(size), mBufferHandle(bufferHandle), mMemoryHandle(memoryHandle), mDescriptorInfo(descriptorInfo) {
    mContext->AddRef();
}

uint8_t* VulkanBuffer::MapBuffer() {
    const vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    return static_cast<uint8_t*>(VKRT_ASSERT_VK(logicalDevice.mapMemory(mMemoryHandle, 0, mSize)));
}

void VulkanBuffer::UnmapBuffer() {
    const vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.unmapMemory(mMemoryHandle);
}

vk::DeviceAddress VulkanBuffer::GetDeviceAddress() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    vk::BufferDeviceAddressInfoKHR bufferAddressInfo = vk::BufferDeviceAddressInfoKHR().setBuffer(mBufferHandle);
    return logicalDevice.getBufferAddress(bufferAddressInfo);
}

VulkanBuffer::~VulkanBuffer() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyBuffer(mBufferHandle);
    logicalDevice.freeMemory(mMemoryHandle);
    mContext->Release();
}

}  // namespace VKRT