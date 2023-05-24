#include "Object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "DebugUtils.h"

namespace VKRT {

Object::Object(ScopedRefPtr<Model> model)
    : mModel(model),
      mTransform(1.0f),
      mPosition(0.0f),
      mEulerRotation(0.0f),
      mScale(1.0f, 1.0f, 1.0f) {
    VKRT_ASSERT(model != nullptr);
}

void Object::SetTranslation(const glm::vec3& position) {
    mPosition = position;
    UpdateTransform();
}

void Object::Translate(const glm::vec3& delta) {
    mPosition += delta;
    UpdateTransform();
}

void Object::Rotate(const glm::vec3& delta) {
    mEulerRotation += delta;
    UpdateTransform();
}

void Object::Scale(const glm::vec3& delta) {
    mScale += delta;
    UpdateTransform();
}

void Object::SetScale(const glm::vec3& scale) {
    mScale = scale;
    UpdateTransform();
}

void Object::UpdateTransform() {
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), mPosition);
    glm::mat4 rotate = glm::eulerAngleYXZ(
        glm::radians(mEulerRotation.y),
        glm::radians(mEulerRotation.x),
        glm::radians(mEulerRotation.z));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), mScale);
    mTransform = translate * rotate * scale;
}

Object::~Object() {}

}  // namespace VKRT