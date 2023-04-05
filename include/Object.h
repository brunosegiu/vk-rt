#pragma once

#include "glm/glm.hpp"

#include "Model.h"

namespace VKRT {

class Object : public RefCountPtr {
public:
    Object(Model* model);

    Model* GetModel() { return mModel; }
    const glm::mat4& GetTransform() { return mTransform; }

    void SetTranslation(const glm::vec3& position);
    void Translate(const glm::vec3& delta);

    void Rotate(const glm::vec3& delta);

    void Scale(const glm::vec3& delta);

    ~Object();

private:
    void UpdateTransform();

    Model* mModel;
    glm::mat4 mTransform;
    glm::vec3 mEulerRotation;
    glm::vec3 mScale;
    glm::vec3 mPosition;
};

}  // namespace VKRT