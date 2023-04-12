#pragma once

#include "Context.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"

namespace VKRT {
class Device;

class Texture : public RefCountPtr {
public:
    Texture(
        Context* context,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        const uint8_t* buffer,
        size_t bufferSize);

    const vk::ImageView& GetImageView() const { return mImageView; }

    static void SetImageLayout(
        vk::CommandBuffer& commandBuffer,
        vk::Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        const vk::ImageSubresourceRange& subresourceRange,
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask);

private:
    Context* mContext;

    vk::Image mImage;
    vk::DeviceMemory mMemory;
    vk::ImageView mImageView;
};
}  // namespace VKRT
