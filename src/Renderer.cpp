#include "Renderer.h"

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
    vk::ImageCreateInfo imageCreateInfo =
        vk::ImageCreateInfo()
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
    mStorageImageMemory = mContext->GetDevice()->AllocateMemory(
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        imageMemReq);
    VKRT_ASSERT_VK(logicalDevice.bindImageMemory(mStorageImage, mStorageImageMemory, 0));

    vk::ImageViewCreateInfo storageImageViewCreateInfo =
        vk::ImageViewCreateInfo()
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
    vk::ImageMemoryBarrier imageMemoryBarrier =
        vk::ImageMemoryBarrier()
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
    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        {},
        nullptr,
        nullptr,
        imageMemoryBarrier);
    VKRT_ASSERT_VK(commandBuffer.end());
    mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
    mContext->GetDevice()->DestroyCommand(commandBuffer);
}

void Renderer::CreateUniformBuffer() {
    {
        mCameraUniformBuffer = mContext->GetDevice()->CreateBuffer(
            sizeof(UniformData),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    {
        const std::vector<Model::Description> descriptions = mScene->GetDescriptions();
        const size_t descriptionsBufferSize = sizeof(Model::Description) * descriptions.size();
        mSceneUniformBuffer = mContext->GetDevice()->CreateBuffer(
            descriptionsBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* buffer = mSceneUniformBuffer->MapBuffer();
        std::copy_n(
            reinterpret_cast<const uint8_t*>(descriptions.data()),
            descriptionsBufferSize,
            buffer);
        mSceneUniformBuffer->UnmapBuffer();
    }

    {
        const std::vector<Light::Description> descriptions = mScene->GetLightDescriptions();
        {
            mLightMetadataUniformBuffer = mContext->GetDevice()->CreateBuffer(
                sizeof(uint32_t),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightMetadataUniformBuffer->MapBuffer();
            const uint32_t lightCount = descriptions.size();
            std::copy_n(reinterpret_cast<const uint8_t*>(&lightCount), sizeof(uint32_t), buffer);
            mLightMetadataUniformBuffer->UnmapBuffer();
        }

        {
            const size_t descriptionsBufferSize = sizeof(Light::Description) * descriptions.size();
            mLightUniformBuffer = mContext->GetDevice()->CreateBuffer(
                descriptionsBufferSize,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightUniformBuffer->MapBuffer();
            std::copy_n(
                reinterpret_cast<const uint8_t*>(descriptions.data()),
                descriptionsBufferSize,
                buffer);
            mLightUniformBuffer->UnmapBuffer();
        }
    }
}

void Renderer::UpdateCameraUniforms(Camera* camera) {
    uint8_t* buffer = mCameraUniformBuffer->MapBuffer();
    UniformData cameraMatrices{
        .viewInverse = glm::inverse(camera->GetViewTransform()),
        .projInverse = glm::inverse(camera->GetProjectionTransform())};
    std::copy_n(reinterpret_cast<uint8_t*>(&cameraMatrices), sizeof(UniformData), buffer);
    mCameraUniformBuffer->UnmapBuffer();
}

void Renderer::UpdateLightUniforms() {
    const std::vector<Light::Description> descriptions = mScene->GetLightDescriptions();
    {
        uint8_t* buffer = mLightMetadataUniformBuffer->MapBuffer();
        const uint32_t lightCount = descriptions.size();
        std::copy_n(reinterpret_cast<const uint8_t*>(&lightCount), sizeof(uint32_t), buffer);
        mLightMetadataUniformBuffer->UnmapBuffer();
    }

    {
        const size_t descriptionsBufferSize = sizeof(Light::Description) * descriptions.size();
        if (descriptionsBufferSize != mLightUniformBuffer->GetBufferSize()) {
            mLightUniformBuffer->Release();
            mLightUniformBuffer = mContext->GetDevice()->CreateBuffer(
                descriptionsBufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightUniformBuffer->MapBuffer();
            std::copy_n(
                reinterpret_cast<const uint8_t*>(descriptions.data()),
                descriptionsBufferSize,
                buffer);
            mLightUniformBuffer->UnmapBuffer();
            vk::WriteDescriptorSet lightUniformBufferWrite =
                vk::WriteDescriptorSet()
                    .setDstSet(mDescriptorSet)
                    .setDstBinding(5)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setBufferInfo(mLightUniformBuffer->GetDescriptorInfo());
            vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
            logicalDevice.updateDescriptorSets(lightUniformBufferWrite, nullptr);
        }
        uint8_t* buffer = mLightUniformBuffer->MapBuffer();
        std::copy_n(
            reinterpret_cast<const uint8_t*>(descriptions.data()),
            descriptionsBufferSize,
            buffer);
        mLightUniformBuffer->UnmapBuffer();
    }
}

void Renderer::CreateDescriptors() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::DescriptorPoolCreateInfo poolCreateInfo =
        vk::DescriptorPoolCreateInfo().setPoolSizes(mPipeline->GetDescriptorSizes()).setMaxSets(1);
    mDescriptorPool = VKRT_ASSERT_VK(logicalDevice.createDescriptorPool(poolCreateInfo));

    vk::DescriptorSetAllocateInfo descriptorAllocateInfo =
        vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(mDescriptorPool)
            .setSetLayouts(mPipeline->GetDescriptorLayout());
    mDescriptorSet =
        VKRT_ASSERT_VK(logicalDevice.allocateDescriptorSets(descriptorAllocateInfo)).front();

    vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo =
        vk::WriteDescriptorSetAccelerationStructureKHR().setAccelerationStructures(
            mScene->GetTLAS());
    vk::WriteDescriptorSet accelerationStructureWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setPNext(&descriptorAccelerationStructureInfo);

    vk::DescriptorImageInfo storageImageInfo = vk::DescriptorImageInfo()
                                                   .setImageView(mStorageImageView)
                                                   .setImageLayout(vk::ImageLayout::eGeneral);
    vk::WriteDescriptorSet imageWrite = vk::WriteDescriptorSet()
                                            .setDstSet(mDescriptorSet)
                                            .setDstBinding(1)
                                            .setDescriptorCount(1)
                                            .setDescriptorType(vk::DescriptorType::eStorageImage)
                                            .setImageInfo(storageImageInfo);

    vk::WriteDescriptorSet cameraUniformBufferWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(2)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setBufferInfo(mCameraUniformBuffer->GetDescriptorInfo());

    vk::WriteDescriptorSet sceneUniformBufferWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(3)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(mSceneUniformBuffer->GetDescriptorInfo());

    vk::WriteDescriptorSet lightMetadataUniformBufferWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(4)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setBufferInfo(mLightMetadataUniformBuffer->GetDescriptorInfo());

    vk::WriteDescriptorSet lightUniformBufferWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(5)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(mLightUniformBuffer->GetDescriptorInfo());

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets{
        accelerationStructureWrite,
        imageWrite,
        cameraUniformBufferWrite,
        sceneUniformBufferWrite,
        lightMetadataUniformBufferWrite,
        lightUniformBufferWrite};

    logicalDevice.updateDescriptorSets(writeDescriptorSets, {});
}

void Renderer::Render(Camera* camera) {
    UpdateCameraUniforms(camera);
    UpdateLightUniforms();

    mContext->GetSwapchain()->AcquireNextImage();

    vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();

    {
        VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));

        commandBuffer.bindPipeline(
            vk::PipelineBindPoint::eRayTracingKHR,
            mPipeline->GetPipelineHandle());
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            mPipeline->GetPipelineLayout(),
            0,
            mDescriptorSet,
            nullptr);

        const vk::Extent2D& imageSize = mContext->GetSwapchain()->GetExtent();
        const RayTracingPipeline::RayTracingTablesRef& tableRef = mPipeline->GetTablesRef();
        commandBuffer.traceRaysKHR(
            tableRef.rayGen,
            tableRef.rayMiss,
            tableRef.rayHit,
            tableRef.callable,
            imageSize.width,
            imageSize.height,
            1,
            mContext->GetDevice()->GetDispatcher());

        vk::Image& currentSwapchainImage = mContext->GetSwapchain()->GetCurrentImage();
        const vk::ImageSubresourceRange subresourceRange =
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        SetImageLayout(
            commandBuffer,
            currentSwapchainImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            subresourceRange,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        SetImageLayout(
            commandBuffer,
            mStorageImage,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferSrcOptimal,
            subresourceRange,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        vk::ImageCopy imageCopyRegion =
            vk::ImageCopy()
                .setSrcSubresource(
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
                .setSrcOffset(vk::Offset3D(0, 0, 0))
                .setDstSubresource(
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
                .setDstOffset(vk::Offset3D(0, 0, 0))
                .setExtent(vk::Extent3D(imageSize.width, imageSize.height, 1));
        commandBuffer.copyImage(
            mStorageImage,
            vk::ImageLayout::eTransferSrcOptimal,
            currentSwapchainImage,
            vk::ImageLayout::eTransferDstOptimal,
            imageCopyRegion);

        SetImageLayout(
            commandBuffer,
            currentSwapchainImage,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            subresourceRange,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        SetImageLayout(
            commandBuffer,
            mStorageImage,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eGeneral,
            subresourceRange,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        VKRT_ASSERT_VK(commandBuffer.end());
    }

    const vk::Queue& queue = mContext->GetDevice()->GetQueue();
    vk::Fence fence = mContext->GetDevice()->CreateFence();
    VKRT_ASSERT_VK(queue.submit(
        vk::SubmitInfo()
            .setCommandBuffers(commandBuffer)
            .setWaitSemaphores(mContext->GetSwapchain()->GetPresentSemaphore())
            .setSignalSemaphores(mContext->GetSwapchain()->GetRenderSemaphore()),
        fence));
    VKRT_ASSERT_VK(mContext->GetDevice()->GetLogicalDevice().waitForFences(
        fence,
        true,
        std::numeric_limits<uint64_t>::max()));
    mContext->GetDevice()->DestroyFence(fence);
    mContext->GetDevice()->DestroyCommand(commandBuffer);

    mContext->GetSwapchain()->Present();
}

void Renderer::SetImageLayout(
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

Renderer::~Renderer() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyDescriptorPool(mDescriptorPool);

    mLightUniformBuffer->Release();
    mLightMetadataUniformBuffer->Release();
    mCameraUniformBuffer->Release();
    mSceneUniformBuffer->Release();

    logicalDevice.destroyImageView(mStorageImageView);
    logicalDevice.destroyImage(mStorageImage);
    logicalDevice.freeMemory(mStorageImageMemory);

    mPipeline->Release();
    mScene->Release();
    mContext->Release();
}

}  // namespace VKRT