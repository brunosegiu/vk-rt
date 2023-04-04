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

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };
    Model(Context* context, const std::vector<Vertex>& vertices, const std::vector<glm::uvec3>& indices);

    struct Description {
        vk::DeviceAddress vertexBufferAddress;
        vk::DeviceAddress indexBufferAddress;
    };
    Description GetDescription();

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