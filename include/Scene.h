#pragma once

#include <vector>

#include "Light.h"
#include "Object.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Context;

class Scene : public RefCountPtr {
public:
    Scene(ScopedRefPtr<Context> context);

    void AddObject(ScopedRefPtr<Object> object);
    void AddLight(ScopedRefPtr<Light> light);

    const vk::AccelerationStructureKHR& GetTLAS() const { return mTLAS; }

    std::vector<Mesh::Description> GetDescriptions();
    std::vector<Light::Proxy> GetLightDescriptions();

    struct MaterialProxy {
        glm::vec3 albedo;
        float roughness;
        float metallic;
        float indexOfRefraction;
        int32_t albedoTextureIndex;
        int32_t roughnessTextureIndex;
    };
    struct SceneMaterials {
        std::vector<MaterialProxy> materials;
        std::vector<ScopedRefPtr<Texture>> textures;
    };
    SceneMaterials GetMaterialProxies();

    void Commit();

    ~Scene();

private:
    ScopedRefPtr<Context> mContext;

    std::vector<ScopedRefPtr<Object>> mObjects;
    std::vector<ScopedRefPtr<Light>> mLights;

    bool mCommitted;
    ScopedRefPtr<VulkanBuffer> mInstanceBuffer;
    ScopedRefPtr<VulkanBuffer> mTLASBuffer;
    vk::AccelerationStructureKHR mTLAS;
    vk::DeviceAddress mTLASAddress;
};
}  // namespace VKRT