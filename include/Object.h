#pragma once

#include "glm/glm.hpp"

#include "Model.h"

namespace VKRT {

class Object : public RefCountPtr {
public:
    Object(Model* model, glm::mat4 transform);

    Model* GetModel() { return mModel; }

    ~Object();

private:
    Model* mModel;
    glm::mat4 mTransform;
};

}  // namespace VKRT