#include "Material.h"

#include "DebugUtils.h"
#include "Device.h"

namespace VKRT {

Material::Material() : Material(glm::vec3(0.5), 1.0f, 0.0f, -1.0f) {}

Material::Material(
    const glm::vec3& albedo,
    float roughness,
    float metallic,
    float indexOfRefraction,
    Texture* albedoTexture,
    Texture* roughnessTexture)
    : mAlbedo(albedo),
      mRoughness(roughness),
      mMetallic(metallic),
      mIndexOfRefraction(indexOfRefraction),
      mAlbedoTexture(albedoTexture),
      mRoughnessTexture(roughnessTexture) {
    if (mAlbedoTexture != nullptr) {
        mAlbedoTexture->AddRef();
    }
    if (mRoughnessTexture != nullptr) {
        mRoughnessTexture->AddRef();
    }
}

Material::~Material() {
    if (mAlbedoTexture != nullptr) {
        mAlbedoTexture->Release();
    }
    if (mRoughnessTexture != nullptr) {
        mRoughnessTexture->Release();
    }
}

}  // namespace VKRT