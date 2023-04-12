#include "Texture.h"

#include "DebugUtils.h"
#include "Device.h"
#include "VulkanBuffer.h"

namespace VKRT {
Texture::Texture(
    Context* context,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    const uint8_t* buffer,
    size_t bufferSize)
    : mContext(context) {
    VulkanBuffer* stagingBuffer = VulkanBuffer::Create(
        mContext,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    uint8_t* stagingData = stagingBuffer->MapBuffer();
    std::copy_n(buffer, bufferSize, stagingData);
    stagingBuffer->UnmapBuffer();

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::ImageCreateInfo imageCreateInfo =
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent(vk::Extent3D{width, height, 1})
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setInitialLayout(vk::ImageLayout::eUndefined);

    mImage = VKRT_ASSERT_VK(logicalDevice.createImage(imageCreateInfo));

    vk::MemoryRequirements imageMemReq = logicalDevice.getImageMemoryRequirements(mImage);
    mMemory = mContext->GetDevice()->AllocateMemory(
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        imageMemReq);
    VKRT_ASSERT_VK(logicalDevice.bindImageMemory(mImage, mMemory, 0));

    vk::ImageViewCreateInfo imageViewCreateInfo =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1))
            .setImage(mImage);
    mImageView = VKRT_ASSERT_VK(logicalDevice.createImageView(imageViewCreateInfo));

    vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
    VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));

    const vk::ImageSubresourceRange subresourceRange =
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    SetImageLayout(
        commandBuffer,
        mImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        subresourceRange,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands);

    vk::BufferImageCopy imageCopy =
        vk::BufferImageCopy()
            .setImageSubresource(
                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
            .setImageExtent(vk::Extent3D{width, height, 1})
            .setBufferOffset(0);

    commandBuffer.copyBufferToImage(
        stagingBuffer->GetBufferHandle(),
        mImage,
        vk::ImageLayout::eTransferDstOptimal,
        imageCopy);

    SetImageLayout(
        commandBuffer,
        mImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        subresourceRange,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands);

    VKRT_ASSERT_VK(commandBuffer.end());
    mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
    mContext->GetDevice()->DestroyCommand(commandBuffer);

    stagingBuffer->Release();
}

void Texture::SetImageLayout(
    vk::CommandBuffer& commandBuffer,
    vk::Image& image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    const vk::ImageSubresourceRange& subresourceRange,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask) {
    vk::ImageMemoryBarrier imageBarrier = vk::ImageMemoryBarrier()
                                              .setOldLayout(oldLayout)
                                              .setNewLayout(newLayout)
                                              .setImage(image)
                                              .setSubresourceRange(subresourceRange);

    switch (oldLayout) {
        case vk::ImageLayout::eUndefined:
            imageBarrier.setSrcAccessMask(vk::AccessFlags{});
            break;
        case vk::ImageLayout::ePreinitialized:
            imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite);
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            break;
    }

    switch (newLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            imageBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            imageBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            imageBarrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
            break;
    }

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, imageBarrier);
}

}  // namespace VKRT