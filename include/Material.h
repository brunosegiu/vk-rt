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
    static constexpr uint32_t OpaqueMask = 0xF0;
    static constexpr uint32_t RefractiveMask = 0x0F;
    static constexpr uint32_t AllMask = OpaqueMask | RefractiveMask;

    Material();

    Material(
        const glm::vec3& albedo,
        float roughness,
        float metallic,
        float indexOfRefraction,
        ScopedRefPtr<Texture> albedoTexture = nullptr,
        ScopedRefPtr<Texture> roughnessTexture = nullptr);

    const glm::vec3 GetAlbedo() const { return mAlbedo; }
    const float GetRoughness() const { return mRoughness; }
    const float GetMetallic() const { return mMetallic; }
    const float GetIndexOfRefraction() const { return mIndexOfRefraction; }
    const ScopedRefPtr<Texture> GetAlbedoTexture() const { return mAlbedoTexture; }
    const ScopedRefPtr<Texture> GetRoughnessTexture() const { return mRoughnessTexture; }

    void SetAlbedo(const glm::vec3& albedo) { mAlbedo = albedo; }
    void SetRoughness(float roughness) { mRoughness = roughness; }
    void SetMetallic(float metallic) { mMetallic = metallic; }
    void SetIndexOfRefraction(float indexOfRefraction) { mIndexOfRefraction = indexOfRefraction; }

    ~Material();

private:
    glm::vec3 mAlbedo;
    float mRoughness;
    float mMetallic;
    float mIndexOfRefraction;

    ScopedRefPtr<Texture> mAlbedoTexture;
    ScopedRefPtr<Texture> mRoughnessTexture;
};
}  // namespace VKRT
