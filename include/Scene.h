#pragma once

#include <vector>

#include "Object.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Context;

class Scene : public RefCountPtr {
public:
    Scene(Context* context);

    void AddObject(Object* object);

    void Commit();

    ~Scene();

private:
    Context* mContext;

    std::vector<Object*> mObjects;

    VulkanBuffer* mInstanceBuffer;
    VulkanBuffer* mTLASBuffer;
    vk::AccelerationStructureKHR mTLAS;
    vk::DeviceAddress mTLASAddress;
};
}  // namespace VKRT