#pragma once

#include "RefCountPtr.h"
#include "Context.h"
#include "Scene.h"
#include "RayTracingPipeline.h"

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