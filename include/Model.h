#pragma once

#include "Context.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Model : public RefCountPtr {
public:
    Model(Context* context);

    vk::DeviceAddress GetBLASAddress() { return mBLASAddress; }

    ~Model();

private:
    Context* mContext;

    VulkanBuffer* mVertexBuffer;
    VulkanBuffer* mIndexBuffer;
    VulkanBuffer* mTransformBuffer;

    VulkanBuffer* mBLASBuffer;
    vk::AccelerationStructureKHR mBLAS;
    vk::DeviceAddress mBLASAddress;
};

}  // namespace VKRT