#pragma once

#include "glm/glm.hpp"

#include "RefCountPtr.h"

namespace VKRT {

class Light : public RefCountPtr {
public:
    Light();

    void SetPosition(const glm::vec3& position) { mPosition = position; }
    void SetIntensity(float intensity) { mIntensity = intensity; }

    const glm::vec3& GetPosition() const { return mPosition; }
    const float& GetIntensity() const { return mIntensity; }

    struct Description {
        glm::vec3 position;
        float intensity;
    };

    ~Light();

private:
    glm::vec3 mPosition;
    float mIntensity;
};

}  // namespace VKRT