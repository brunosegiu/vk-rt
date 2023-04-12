#pragma once

#include "glm/glm.hpp"

#include "Context.h"
#include "Material.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Mesh : public RefCountPtr {
public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };
    Mesh(
        Context* context,
        const std::vector<Vertex>& vertices,
        const std::vector<glm::uvec3>& indices,
        Material* material);

    struct Description {
        vk::DeviceAddress vertexBufferAddress;
        vk::DeviceAddress indexBufferAddress;
    };
    Description GetDescription() const;

    vk::DeviceAddress GetBLASAddress() const { return mBLASAddress; }
    const Material* GetMaterial() const { return mMaterial; }

    ~Mesh();

private:
    Context* mContext;

    VulkanBuffer* mVertexBuffer;
    VulkanBuffer* mIndexBuffer;
    VulkanBuffer* mTransformBuffer;

    VulkanBuffer* mBLASBuffer;
    vk::AccelerationStructureKHR mBLAS;
    vk::DeviceAddress mBLASAddress;

    Material* mMaterial;
};

}  // namespace VKRT