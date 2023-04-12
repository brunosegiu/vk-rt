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
    void CreateMaterialUniforms(const Scene::SceneMaterials& materialInfo);
    void CreateDescriptors(const Scene::SceneMaterials& materialInfo);
    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };
    void UpdateCameraUniforms(Camera* camera);
    void UpdateLightUniforms();

    Context* mContext;
    Scene* mScene;

    vk::Image mStorageImage;
    vk::DeviceMemory mStorageImageMemory;
    vk::ImageView mStorageImageView;

    VulkanBuffer* mCameraUniformBuffer;
    VulkanBuffer* mSceneUniformBuffer;
    VulkanBuffer* mLightMetadataUniformBuffer;
    VulkanBuffer* mLightUniformBuffer;
    VulkanBuffer* mMaterialsBuffer;

    RayTracingPipeline* mPipeline;
    vk::DescriptorPool mDescriptorPool;
    vk::DescriptorSet mDescriptorSet;

    vk::Sampler mTextureSampler;
};

}  // namespace VKRT