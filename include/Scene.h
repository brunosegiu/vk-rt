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
    const vk::AccelerationStructureKHR& GetTLAS() const { return mTLAS; }

    std::vector<Model::Description> GetDescriptions();

    void Commit();

    ~Scene();

private:
    Context* mContext;

    std::vector<Object*> mObjects;

    bool mCommitted;
    VulkanBuffer* mInstanceBuffer;
    VulkanBuffer* mTLASBuffer;
    vk::AccelerationStructureKHR mTLAS;
    vk::DeviceAddress mTLASAddress;
};
}  // namespace VKRT