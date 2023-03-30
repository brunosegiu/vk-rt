#pragma once

#include "glm/glm.hpp"

#include "Context.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Model : public RefCountPtr {
public:
    static Model* Load(Context* context, const std::string& path);

    Model(Context* context, const std::vector<glm::vec3>& vertices, const std::vector<glm::uvec3>& indices);

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