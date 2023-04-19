#include "Renderer.h"

#include "DebugUtils.h"
#include "Texture.h"

namespace VKRT {
Renderer::Renderer(Context* context, Scene* scene) : mContext(context), mScene(scene) {
    mContext->AddRef();
    mScene->AddRef();
    Scene::SceneMaterials materials = mScene->GetMaterialProxies();
    mPipeline = new RayTracingPipeline(mContext);
    CreateStorageImage();
    CreateUniformBuffer();
    CreateMaterialUniforms(materials);
    CreateDescriptors(materials);
}

void Renderer::CreateStorageImage() {
    Swapchain* swapchain = mContext->GetSwapchain();
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    const vk::Format swapchainFormat = swapchain->GetFormat();
    const uint32_t swapchainWidth = swapchain->GetExtent().width;
    const uint32_t swapchainHeight = swapchain->GetExtent().height;

    mStorageTexture = new Texture(
        mContext,
        swapchainWidth,
        swapchainHeight,
        swapchainFormat,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage);

    vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
    VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));
    mStorageTexture->SetImageLayout(
        commandBuffer,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands);
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
        const std::vector<Mesh::Description> descriptions = mScene->GetDescriptions();
        const size_t descriptionsBufferSize = sizeof(Mesh::Description) * descriptions.size();
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
        const std::vector<Light::Proxy> lightProxies = mScene->GetLightDescriptions();
        {
            mLightMetadataUniformBuffer = mContext->GetDevice()->CreateBuffer(
                sizeof(uint32_t),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightMetadataUniformBuffer->MapBuffer();
            const uint32_t lightCount = lightProxies.size();
            std::copy_n(reinterpret_cast<const uint8_t*>(&lightCount), sizeof(uint32_t), buffer);
            mLightMetadataUniformBuffer->UnmapBuffer();
        }

        {
            const size_t lightProxiesBufferSize = sizeof(Light::Proxy) * lightProxies.size();
            mLightUniformBuffer = mContext->GetDevice()->CreateBuffer(
                lightProxiesBufferSize,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightUniformBuffer->MapBuffer();
            std::copy_n(
                reinterpret_cast<const uint8_t*>(lightProxies.data()),
                lightProxiesBufferSize,
                buffer);
            mLightUniformBuffer->UnmapBuffer();
        }
    }
}

void Renderer::CreateMaterialUniforms(const Scene::SceneMaterials& materialInfo) {
    {
        vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
        const float anisotropy =
            mContext->GetDevice()->GetDeviceProperties().limits.maxSamplerAnisotropy;
        vk::SamplerCreateInfo samplerCreateInfo =
            vk::SamplerCreateInfo()
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setMipLodBias(0.0f)
                .setCompareOp(vk::CompareOp::eNever)
                .setMinLod(0.0f)
                .setMaxLod(0.0f)
                .setAnisotropyEnable(true)
                .setMaxAnisotropy(anisotropy);
        mTextureSampler = VKRT_ASSERT_VK(logicalDevice.createSampler(samplerCreateInfo));
    }

    {
        const size_t materialBufferSize =
            sizeof(Scene::MaterialProxy) * materialInfo.materials.size();
        mMaterialsBuffer = mContext->GetDevice()->CreateBuffer(
            materialBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* buffer = mMaterialsBuffer->MapBuffer();
        std::copy_n(
            reinterpret_cast<const uint8_t*>(materialInfo.materials.data()),
            materialBufferSize,
            buffer);
        mMaterialsBuffer->UnmapBuffer();
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
    const std::vector<Light::Proxy> lightProxies = mScene->GetLightDescriptions();
    {
        uint8_t* buffer = mLightMetadataUniformBuffer->MapBuffer();
        const uint32_t lightCount = lightProxies.size();
        std::copy_n(reinterpret_cast<const uint8_t*>(&lightCount), sizeof(uint32_t), buffer);
        mLightMetadataUniformBuffer->UnmapBuffer();
    }

    {
        const size_t lightProxiesBufferSize = sizeof(Light::Proxy) * lightProxies.size();
        if (lightProxiesBufferSize != mLightUniformBuffer->GetBufferSize()) {
            mLightUniformBuffer->Release();
            mLightUniformBuffer = mContext->GetDevice()->CreateBuffer(
                lightProxiesBufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* buffer = mLightUniformBuffer->MapBuffer();
            std::copy_n(
                reinterpret_cast<const uint8_t*>(lightProxies.data()),
                lightProxiesBufferSize,
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
            reinterpret_cast<const uint8_t*>(lightProxies.data()),
            lightProxiesBufferSize,
            buffer);
        mLightUniformBuffer->UnmapBuffer();
    }
}

void Renderer::CreateDescriptors(const Scene::SceneMaterials& materialInfo) {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::DescriptorPoolCreateInfo poolCreateInfo =
        vk::DescriptorPoolCreateInfo().setPoolSizes(mPipeline->GetDescriptorSizes()).setMaxSets(1);
    mDescriptorPool = VKRT_ASSERT_VK(logicalDevice.createDescriptorPool(poolCreateInfo));

    std::vector<uint32_t> descriptorCounts{static_cast<uint32_t>(materialInfo.textures.size())};
    vk::DescriptorSetVariableDescriptorCountAllocateInfo dynamicCountInfo =
        vk::DescriptorSetVariableDescriptorCountAllocateInfo().setDescriptorCounts(
            descriptorCounts);

    vk::DescriptorSetAllocateInfo descriptorAllocateInfo =
        vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(mDescriptorPool)
            .setSetLayouts(mPipeline->GetDescriptorLayout())
            .setPNext(&dynamicCountInfo);
    mDescriptorSet = VKRT_ASSERT_VK(logicalDevice.allocateDescriptorSets(
                                        descriptorAllocateInfo,
                                        mContext->GetDevice()->GetDispatcher()))
                         .front();

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
                                                   .setImageView(mStorageTexture->GetImageView())
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

    auto sampler = vk::DescriptorImageInfo().setSampler(mTextureSampler);
    vk::WriteDescriptorSet samplerWrite = vk::WriteDescriptorSet()
                                              .setDstSet(mDescriptorSet)
                                              .setDstBinding(6)
                                              .setDescriptorCount(1)
                                              .setDescriptorType(vk::DescriptorType::eSampler)
                                              .setImageInfo(sampler);

    vk::WriteDescriptorSet materialsWrite =
        vk::WriteDescriptorSet()
            .setDstSet(mDescriptorSet)
            .setDstBinding(7)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setBufferInfo(mMaterialsBuffer->GetDescriptorInfo());

    std::vector<vk::DescriptorImageInfo> imageInfos;
    for (const Texture* texture : materialInfo.textures) {
        imageInfos.push_back(vk::DescriptorImageInfo()
                                 .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                 .setImageView(texture->GetImageView())
                                 .setSampler(nullptr));
    }

    vk::WriteDescriptorSet texturesWrite = vk::WriteDescriptorSet()
                                               .setDstSet(mDescriptorSet)
                                               .setDstBinding(8)
                                               .setDescriptorType(vk::DescriptorType::eSampledImage)
                                               .setImageInfo(imageInfos)
                                               .setDstArrayElement(0)
                                               .setPBufferInfo(nullptr)
                                               .setPTexelBufferView(nullptr);

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets{
        accelerationStructureWrite,
        imageWrite,
        cameraUniformBufferWrite,
        sceneUniformBufferWrite,
        lightMetadataUniformBufferWrite,
        lightUniformBufferWrite,
        samplerWrite,
        materialsWrite,
        texturesWrite};

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

        Texture* currentSwapchainImage = mContext->GetSwapchain()->GetCurrentImage();
        const vk::ImageSubresourceRange subresourceRange =
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        currentSwapchainImage->SetImageLayout(
            commandBuffer,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        mStorageTexture->SetImageLayout(
            commandBuffer,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eTransferSrcOptimal,
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
            mStorageTexture->GetImage(),
            vk::ImageLayout::eTransferSrcOptimal,
            currentSwapchainImage->GetImage(),
            vk::ImageLayout::eTransferDstOptimal,
            imageCopyRegion);

        currentSwapchainImage->SetImageLayout(
            commandBuffer,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        mStorageTexture->SetImageLayout(
            commandBuffer,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands);

        VKRT_ASSERT_VK(commandBuffer.end());
    }

    const vk::Queue& queue = mContext->GetDevice()->GetQueue();
    vk::Fence fence = mContext->GetDevice()->CreateFence();

    std::vector<vk::Semaphore> waitSemaphores{mContext->GetSwapchain()->GetPresentSemaphore()};
    std::vector<vk::Semaphore> signalSemaphores{mContext->GetSwapchain()->GetRenderSemaphore()};
    std::vector<vk::PipelineStageFlags> waitStages{vk::PipelineStageFlagBits::eAllCommands};
    VKRT_ASSERT_VK(queue.submit(
        vk::SubmitInfo()
            .setCommandBuffers(commandBuffer)
            .setWaitSemaphores(waitSemaphores)
            .setSignalSemaphores(signalSemaphores)
            .setWaitDstStageMask(waitStages),
        fence));
    VKRT_ASSERT_VK(mContext->GetDevice()->GetLogicalDevice().waitForFences(
        fence,
        true,
        std::numeric_limits<uint64_t>::max()));

    mContext->GetSwapchain()->Present();

    mContext->GetDevice()->DestroyFence(fence);
    mContext->GetDevice()->DestroyCommand(commandBuffer);
}

Renderer::~Renderer() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyDescriptorPool(mDescriptorPool);

    mMaterialsBuffer->Release();
    logicalDevice.destroySampler(mTextureSampler);
    mLightUniformBuffer->Release();
    mLightMetadataUniformBuffer->Release();
    mCameraUniformBuffer->Release();
    mSceneUniformBuffer->Release();

    mStorageTexture->Release();

    mPipeline->Release();
    mScene->Release();
    mContext->Release();
}

}  // namespace VKRT