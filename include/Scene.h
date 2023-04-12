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
    Scene(Context* context);

    void AddObject(Object* object);
    void AddLight(Light* light);

    const vk::AccelerationStructureKHR& GetTLAS() const { return mTLAS; }

    std::vector<Model::Description> GetDescriptions();
    std::vector<Light::Description> GetLightDescriptions();

    struct MaterialProxy {
        glm::vec3 albedo;
        float roughness;
        int32_t albedoTextureIndex;
        int32_t roughnessTextureIndex;
    };
    struct SceneMaterials {
        std::vector<MaterialProxy> materials;
        std::vector<const Texture*> textures;
    };
    SceneMaterials GetMaterialProxies();

    void Commit();

    ~Scene();

private:
    Context* mContext;

    std::vector<Object*> mObjects;
    std::vector<Light*> mLights;

    bool mCommitted;
    VulkanBuffer* mInstanceBuffer;
    VulkanBuffer* mTLASBuffer;
    vk::AccelerationStructureKHR mTLAS;
    vk::DeviceAddress mTLASAddress;
};
}  // namespace VKRT