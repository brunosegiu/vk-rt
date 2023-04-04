#pragma once

#include "Camera.h"
#include "Context.h"
#include "RayTracingPipeline.h"
#include "RefCountPtr.h"
#include "Scene.h"

namespace VKRT {
class Renderer : public RefCountPtr {
public:
    Renderer(Context* context, Scene* scene);

    void Render(Camera* camera);

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
    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };
    void UpdateCameraUniforms(Camera* camera);

    Context* mContext;
    Scene* mScene;

    vk::Image mStorageImage;
    vk::DeviceMemory mStorageImageMemory;
    vk::ImageView mStorageImageView;

    VulkanBuffer* mCameraUniformBuffer;
    VulkanBuffer* mSceneUniformBuffer;

    RayTracingPipeline* mPipeline;
    vk::DescriptorPool mDescriptorPool;
    vk::DescriptorSet mDescriptorSet;
};

}  // namespace VKRT