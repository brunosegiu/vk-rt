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
        ScopedRefPtr<Context> context,
        const std::vector<Vertex>& vertices,
        const std::vector<glm::uvec3>& indices,
        ScopedRefPtr<Material> material);

    struct Description {
        vk::DeviceAddress vertexBufferAddress;
        vk::DeviceAddress indexBufferAddress;
    };
    Description GetDescription() const;

    vk::DeviceAddress GetBLASAddress() const { return mBLASAddress; }
    const ScopedRefPtr<Material> GetMaterial() const { return mMaterial; }
    ScopedRefPtr<Material> GetMaterial() { return mMaterial; }

    ~Mesh();

private:
    ScopedRefPtr<Context> mContext;

    ScopedRefPtr<VulkanBuffer> mVertexBuffer;
    ScopedRefPtr<VulkanBuffer> mIndexBuffer;
    ScopedRefPtr<VulkanBuffer> mTransformBuffer;

    ScopedRefPtr<VulkanBuffer> mBLASBuffer;
    vk::AccelerationStructureKHR mBLAS;
    vk::DeviceAddress mBLASAddress;

    ScopedRefPtr<Material> mMaterial;
};

}  // namespace VKRT