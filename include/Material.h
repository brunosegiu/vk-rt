#pragma once

#include <glm/glm.hpp>

#include "Context.h"
#include "RefCountPtr.h"
#include "Texture.h"
#include "VulkanBase.h"

namespace VKRT {
class Device;

class Material : public RefCountPtr {
public:
    Material();

    Material(
        const glm::vec3& albedo,
        float roughness,
        Texture* albedoTexture = nullptr,
        Texture* roughnessTexture = nullptr);

    const glm::vec3 GetAlbedo() const { return mAlbedo; }
    const float GetRoughness() const { return mRoughness; }
    const Texture* GetAlbedoTexture() const { return mAlbedoTexture; }
    const Texture* GetRoughnessTexture() const { return mRoughnessTexture; }

    void SetRoughness(float roughness) { mRoughness = roughness; }

    ~Material();

private:
    glm::vec3 mAlbedo;
    float mRoughness;

    Texture* mAlbedoTexture;
    Texture* mRoughnessTexture;
};
}  // namespace VKRT
