#include "Object.h"

namespace VKRT {

Object::Object(Model* model, glm::mat4 transform) : mModel(model), mTransform(transform) {
    mModel->AddRef();
}

Object::~Object() {
    mModel->Release();
}

}  // namespace VKRT