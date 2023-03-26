#include "Renderer.h"

#include "glm/gtc/matrix_transform.hpp"

#include "DebugUtils.h"

namespace VKRT {
Renderer::Renderer(Context* context, Scene* scene) : mContext(context), mScene(scene) {
    mContext->AddRef();
    mScene->AddRef();
    mPipeline = new RayTracingPipeline(mContext);
    CreateStorageImage();
    CreateUniformBuffer();
    CreateDescriptors();
}

void Renderer::CreateStorageImage() {
    Swapchain* swapchain = mContext->GetSwapchain();
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
                                              .setImageType(vk::ImageType::e2D)
                                              .setFormat(swapchain->GetFormat())
                                              .setExtent(vk::Extent3D{swapchain->GetExtent(), 1})
                                              .setMipLevels(1)
                                              .setArrayLayers(1)
                                              .setSamples(vk::SampleCountFlagBits::e1)
                                              .setTiling(vk::ImageTiling::eOptimal)
                                              .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage)
                                              .setInitialLayout(vk::ImageLayout::eUndefined);
    mStorageImage = VKRT_ASSERT_VK(logicalDevice.createImage(imageCreateInfo));

    vk::MemoryRequirements imageMemReq = logicalDevice.getImageMemoryRequirements(mStorageImage);
    mStorageImageMemory = mContext->GetDevice()->AllocateMemory(vk::MemoryPropertyFlagBits::eDeviceLocal, imageMemReq);
    VKRT_ASSERT_VK(logicalDevice.bindImageMemory(mStorageImage, mStorageImageMemory, 0));

    vk::ImageViewCreateInfo storageImageViewCreateInfo = vk::ImageViewCreateInfo()
                                                             .setViewType(vk::ImageViewType::e2D)
                                                             .setFormat(swapchain->GetFormat())
                                                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                                      .setBaseMipLevel(0)
                                                                                      .setLevelCount(1)
                                                                                      .setBaseArrayLayer(0)
                                                                                      .setLayerCount(1))
                                                             .setImage(mStorageImage);
    mStorageImageView = VKRT_ASSERT_VK(logicalDevice.createImageView(storageImageViewCreateInfo));

    vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
    VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));
    vk::ImageMemoryBarrier imageMemoryBarrier = vk::ImageMemoryBarrier()
                                                    .setOldLayout(vk::ImageLayout::eUndefined)
                                                    .setNewLayout(vk::ImageLayout::eGeneral)
                                                    .setSubresourceRange(vk::ImageSubresourceRange()
                                                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                             .setBaseMipLevel(0)
                                                                             .setLevelCount(1)
                                                                             .setBaseArrayLayer(0)
                                                                             .setLayerCount(1))
                                                    .setSrcAccessMask({})
                                                    .setImage(mStorageImage);
    commandBuffer
        .pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, imageMemoryBarrier);
    VKRT_ASSERT_VK(commandBuffer.end());
    mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
    mContext->GetDevice()->DestroyCommand(commandBuffer);
}

void Renderer::CreateUniformBuffer() {
    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };
    mUniformBuffer = mContext->GetDevice()->CreateBuffer(
        sizeof(UniformData),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    uint8_t* buffer = mUniformBuffer->MapBuffer();
    UniformData cameraMatrices{
        .viewInverse = glm::inverse(glm::mat4(1.0f)),
        .projInverse = glm::inverse(glm::perspective(glm::radians(60.0), 1.777, 0.1, 512.0))};
    std::copy_n(reinterpret_cast<uint8_t*>(&cameraMatrices), sizeof(UniformData), buffer);
    mUniformBuffer->UnmapBuffer();
}

void Renderer::CreateDescriptors() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo().setPoolSizes(mPipeline->GetDescriptorSizes()).setMaxSets(1);
    mDescriptorPool = VKRT_ASSERT_VK(logicalDevice.createDescriptorPool(poolCreateInfo));

    vk::DescriptorSetAllocateInfo descriptorAllocateInfo =
        vk::DescriptorSetAllocateInfo().setDescriptorPool(mDescriptorPool).setSetLayouts(mPipeline->GetDescriptorLayout());
    mDescriptorSet = VKRT_ASSERT_VK(logicalDevice.allocateDescriptorSets(descriptorAllocateInfo)).front();

    vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo =
        vk::WriteDescriptorSetAccelerationStructureKHR().setAccelerationStructures(mScene->GetTLAS());
    vk::WriteDescriptorSet accelerationStructureWrite = vk::WriteDescriptorSet()
                                                            .setDstSet(mDescriptorSet)
                                                            .setDstBinding(0)
                                                            .setDescriptorCount(1)
                                                            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                                                            .setPNext(&descriptorAccelerationStructureInfo);

    vk::DescriptorImageInfo storageImageInfo = vk::DescriptorImageInfo().setImageView(mStorageImageView).setImageLayout(vk::ImageLayout::eGeneral);
    vk::WriteDescriptorSet imageWrite = vk::WriteDescriptorSet()
                                            .setDstSet(mDescriptorSet)
                                            .setDstBinding(1)
                                            .setDescriptorCount(1)
                                            .setDescriptorType(vk::DescriptorType::eStorageImage)
                                            .setImageInfo(storageImageInfo);

    vk::WriteDescriptorSet unformBufferWrite = vk::WriteDescriptorSet()
                                                   .setDstSet(mDescriptorSet)
                                                   .setDstBinding(2)
                                                   .setDescriptorCount(1)
                                                   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                                   .setBufferInfo(mUniformBuffer->GetDescriptorInfo());

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets{accelerationStructureWrite, imageWrite, unformBufferWrite};
    logicalDevice.updateDescriptorSets(writeDescriptorSets, {});
}

void Renderer::Render() {
    mContext->GetSwapchain()->AcquireNextImage();

}

Renderer::~Renderer() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyDescriptorPool(mDescriptorPool);

    mUniformBuffer->Release();

    logicalDevice.destroyImageView(mStorageImageView);
    logicalDevice.destroyImage(mStorageImage);
    logicalDevice.freeMemory(mStorageImageMemory);

    mPipeline->Release();
    mScene->Release();
    mContext->Release();
}

}  // namespace VKRT