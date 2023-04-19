#pragma once

#include "glm/glm.hpp"

#include "RefCountPtr.h"

namespace VKRT {

class Light : public RefCountPtr {
public:
    Light();

    void SetIntensity(float intensity) { mIntensity = intensity; }
    const float& GetIntensity() const { return mIntensity; }

    enum class Type : uint32_t { Directional = 0, Point = 1 };

    struct Proxy {
        Type type;
        glm::vec3 directionOrPosition;
        float intensity;
    };

    virtual Proxy GetProxy() = 0;

    virtual ~Light();

private:
    float mIntensity;
};

class DirectionalLight : public Light {
public:
    DirectionalLight();

    void SetDirection(const glm::vec3& direction) { mDirection = direction; }
    const glm::vec3& GetDirection() const { return mDirection; }

    Proxy GetProxy() override;

    ~DirectionalLight();

private:
    glm::vec3 mDirection;
};

class PointLight : public Light {
public:
    PointLight();

    void SetPosition(const glm::vec3& position) { mPosition = position; }
    const glm::vec3& GetPosition() const { return mPosition; }

    Proxy GetProxy() override;

    ~PointLight();

private:
    glm::vec3 mPosition;
};

}  // namespace VKRT