#pragma once

#include "VulkanBase.h"
#include "Model.h"
#include "RefCountPtr.h"

namespace VKRT {

class Context;

class Scene : public RefCountPtr {
public:
    Scene(Context* context);

    ~Scene();
};
}  // namespace VKRT