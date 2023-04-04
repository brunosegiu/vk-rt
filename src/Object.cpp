#include "Object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VKRT {

Object::Object(Model* model) : mModel(model), mTransform(1.0f), mPosition(0.0f), mEulerRotation(0.0f) {
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

void Object::UpdateTransform() {
    glm::mat4 rotationTransform = glm::mat4(1.0f);
    rotationTransform = glm::rotate(rotationTransform, glm::radians(mEulerRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationTransform = glm::rotate(rotationTransform, glm::radians(mEulerRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationTransform = glm::rotate(rotationTransform, glm::radians(mEulerRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 translationTransform = glm::translate(glm::mat4(1.0f), mPosition);

    mTransform = rotationTransform * translationTransform;
}

Object::~Object() {
    mModel->Release();
}

}  // namespace VKRT