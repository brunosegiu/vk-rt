#include "Object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VKRT {

Object::Object(Model* model)
    : mModel(model),
      mTransform(1.0f),
      mPosition(0.0f),
      mEulerRotation(0.0f),
      mScale(1.0f, 1.0f, 1.0f) {
    mModel->AddRef();
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
    mTransform = glm::translate(glm::mat4(1.0f), mPosition);
    mTransform =
        glm::rotate(mTransform, glm::radians(mEulerRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    mTransform =
        glm::rotate(mTransform, glm::radians(mEulerRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    mTransform =
        glm::rotate(mTransform, glm::radians(mEulerRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    mTransform = glm::scale(mTransform, mScale);
}

Object::~Object() {
    mModel->Release();
}

}  // namespace VKRT