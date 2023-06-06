#pragma once

#include "Context.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"

namespace VKRT {
class Device;

class Texture : public RefCountPtr {
public:
    Texture(
        ScopedRefPtr<Context> context,
        uint32_t width,
        uint32_t height,
        uint32_t layers,
        vk::Format format,
        vk::ImageUsageFlags usageFlags,
        vk::Image image = nullptr);

    Texture(
        ScopedRefPtr<Context> context,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageUsageFlags usageFlags,
        vk::Image image = nullptr);

    Texture(
        ScopedRefPtr<Context> context,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        const uint8_t* buffer,
        size_t bufferSize);

    const vk::ImageView& GetImageView() const { return mImageView; }
    const vk::Image& GetImage() const { return mImage; }

    void SetImageLayout(
        vk::CommandBuffer& commandBuffer,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask);

    ~Texture();

private:
    ScopedRefPtr<Context> mContext;

    vk::Image mImage;
    vk::DeviceMemory mMemory;
    vk::ImageView mImageView;
    bool ownsImage;
    uint32_t mWidth, mHeight, mLayers;
};
}  // namespace VKRT
