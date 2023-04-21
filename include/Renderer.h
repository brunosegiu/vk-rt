#pragma once

#include "Camera.h"
#include "Context.h"
#include "RayTracingPipeline.h"
#include "RefCountPtr.h"
#include "Scene.h"

namespace VKRT {
class Renderer : public RefCountPtr {
public:
    Renderer(ScopedRefPtr<Context> context, ScopedRefPtr<Scene> scene);

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

    ScopedRefPtr<Context> mContext;
    ScopedRefPtr<Scene> mScene;

    ScopedRefPtr<Texture> mStorageTexture;

    ScopedRefPtr<VulkanBuffer> mCameraUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mSceneUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mLightMetadataUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mLightUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mMaterialsBuffer;

    ScopedRefPtr<RayTracingPipeline> mPipeline;
    vk::DescriptorPool mDescriptorPool;
    vk::DescriptorSet mDescriptorSet;

    vk::Sampler mTextureSampler;
};

}  // namespace VKRT