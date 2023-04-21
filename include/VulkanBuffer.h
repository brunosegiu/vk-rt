#pragma once

#include "Context.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"

namespace VKRT {
class Device;

class VulkanBuffer : public RefCountPtr {
public:
    static ScopedRefPtr<VulkanBuffer> Create(
        ScopedRefPtr<Context> context,
        const vk::DeviceSize& size,
        const vk::BufferUsageFlags& usageFlags,
        const vk::MemoryPropertyFlags& memoryFlags,
        const vk::MemoryAllocateFlags& memoryAllocateFlags = {});

    const vk::DeviceSize& GetBufferSize() const { return mSize; }
    const vk::Buffer& GetBufferHandle() const { return mBufferHandle; }
    const vk::DescriptorBufferInfo& GetDescriptorInfo() const { return mDescriptorInfo; }

    uint8_t* MapBuffer();
    void UnmapBuffer();

    vk::DeviceAddress GetDeviceAddress();

private:
    VulkanBuffer(
        ScopedRefPtr<Context> context,
        vk::DeviceSize size,
        vk::Buffer bufferHandle,
        vk::DeviceMemory memoryHandle,
        vk::DescriptorBufferInfo descriptorInfo);

    ~VulkanBuffer() override;

    ScopedRefPtr<Context> mContext;
    vk::DeviceSize mSize;
    vk::Buffer mBufferHandle;
    vk::DeviceMemory mMemoryHandle;
    vk::DescriptorBufferInfo mDescriptorInfo;
};
}  // namespace VKRT
