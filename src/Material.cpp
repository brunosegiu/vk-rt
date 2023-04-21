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
    ScopedRefPtr<Texture> albedoTexture,
    ScopedRefPtr<Texture> roughnessTexture)
    : mAlbedo(albedo),
      mRoughness(roughness),
      mMetallic(metallic),
      mIndexOfRefraction(indexOfRefraction),
      mAlbedoTexture(albedoTexture),
      mRoughnessTexture(roughnessTexture) {}

Material::~Material() {}

}  // namespace VKRT