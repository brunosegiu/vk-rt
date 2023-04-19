#include "Light.h"

namespace VKRT {
Light::Light() : mIntensity(0.0f) {}

Light::~Light() {}

DirectionalLight::DirectionalLight() : Light(), mDirection(glm::vec3(0.0f, -1.0f, 0.0f)) {}

Light::Proxy DirectionalLight::GetProxy() {
    return Light::Proxy{
        .type = Light::Type::Directional,
        .directionOrPosition = mDirection,
        .intensity = GetIntensity(),
    };
}

DirectionalLight::~DirectionalLight() {}

PointLight::PointLight() : Light(), mPosition(glm::vec3(0.0f)) {}

Light::Proxy PointLight::GetProxy() {
    return Light::Proxy{
        .type = Light::Type::Point,
        .directionOrPosition = mPosition,
        .intensity = GetIntensity(),
    };
}

PointLight::~PointLight() {}

}  // namespace VKRT