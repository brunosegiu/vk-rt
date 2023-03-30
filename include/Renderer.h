#pragma once

#include "Context.h"
#include "RayTracingPipeline.h"
#include "RefCountPtr.h"
#include "Scene.h"

namespace VKRT {
class Renderer : public RefCountPtr {
public:
    Renderer(Context* context, Scene* scene);

    void Render();

    ~Renderer();

private:
    void CreateStorageImage();
    void CreateUniformBuffer();
    void CreateDescriptors();
    void SetImageLayout(
        vk::CommandBuffer& commandBuffer,
        vk::Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        const vk::ImageSubresourceRange& subresourceRange,
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask);

    Context* mContext;
    Scene* mScene;

    vk::Image mStorageImage;
    vk::DeviceMemory mStorageImageMemory;
    vk::ImageView mStorageImageView;

    VulkanBuffer* mUniformBuffer;

    RayTracingPipeline* mPipeline;
    vk::DescriptorPool mDescriptorPool;
    vk::DescriptorSet mDescriptorSet;
};

}  // namespace VKRT