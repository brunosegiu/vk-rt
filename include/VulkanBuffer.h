#pragma once

#include "RefCountPtr.h"
#include "VulkanBase.h"

namespace VKRT {
class Device;

class VulkanBuffer : public RefCountPtr {
public:
    static VulkanBuffer* Create(
        Device* device,
        const vk::DeviceSize& size,
        const vk::BufferUsageFlags& usageFlags,
        const vk::MemoryPropertyFlags& memoryFlags);

    const vk::DeviceSize& GetBufferSize() const { return mSize; }
    const vk::Buffer& GetBufferHandle() const { return mBufferHandle; }
    const vk::DescriptorBufferInfo& GetDescriptorInfo() const { return mDescriptorInfo; }

    uint8_t* MapBuffer();
    void UnmapBuffer();

private:
    VulkanBuffer(
        Device* device,
        vk::DeviceSize size,
        vk::Buffer bufferHandle,
        vk::DeviceMemory memoryHandle,
        vk::DescriptorBufferInfo descriptorInfo);

    ~VulkanBuffer() override;

    Device* mDevice;
    vk::DeviceSize mSize;
    vk::Buffer mBufferHandle;
    vk::DeviceMemory mMemoryHandle;
    vk::DescriptorBufferInfo mDescriptorInfo;
};
}  // namespace VKRT
