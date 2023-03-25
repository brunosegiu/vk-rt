#include "Model.h"

#include "DebugUtils.h"
#include "glm/glm.hpp"

namespace VKRT {

Model::Model(Context* context) : mContext(context) {
    mContext->AddRef();

    std::vector<glm::vec3> vertices{{1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    std::vector<glm::uvec3> indices{{0, 1, 2}};
    uint32_t triangleCount = indices.size() / 3;
    vk::TransformMatrixKHR transformMatrix = std::array<std::array<float, 4>, 3>{
        std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f},
        std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f},
        std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}};

    {
        const size_t vertexBufferSize = vertices.size() * sizeof(glm::vec3);
        mVertexBuffer = context->GetDevice()->CreateBuffer(
            vertexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* bufferData = mVertexBuffer->MapBuffer();
        std::copy_n(bufferData, vertexBufferSize, reinterpret_cast<uint8_t*>(vertices.data()));
        mVertexBuffer->UnmapBuffer();
    }

    {
        const size_t indexBufferSize = indices.size() * sizeof(glm::uvec3);
        mIndexBuffer = context->GetDevice()->CreateBuffer(
            indexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* bufferData = mIndexBuffer->MapBuffer();
        std::copy_n(bufferData, indexBufferSize, reinterpret_cast<uint8_t*>(indices.data()));
        mIndexBuffer->UnmapBuffer();
    }

    {
        mTransformBuffer = context->GetDevice()->CreateBuffer(
            sizeof(vk::TransformMatrixKHR),
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* bufferData = mTransformBuffer->MapBuffer();
        std::copy_n(bufferData, sizeof(vk::TransformMatrixKHR), reinterpret_cast<uint8_t*>(&transformMatrix));
        mTransformBuffer->UnmapBuffer();
    }

    vk::AccelerationStructureGeometryTrianglesDataKHR triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
                                                                         .setVertexFormat(vk::Format::eR32G32B32A32Sfloat)
                                                                         .setVertexData(mVertexBuffer->GetDeviceAddress())
                                                                         .setMaxVertex(vertices.size())
                                                                         .setVertexStride(sizeof(glm::vec3))
                                                                         .setIndexType(vk::IndexType::eUint32)
                                                                         .setIndexData(mIndexBuffer->GetDeviceAddress())
                                                                         .setTransformData(mTransformBuffer->GetDeviceAddress());

    vk::AccelerationStructureGeometryKHR accelerationStructureGeometry = vk::AccelerationStructureGeometryKHR()
                                                                             .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
                                                                             .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                                                                             .setGeometry(triangleData);

    vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
        vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(accelerationStructureGeometry);

    vk::Device& logicalDevice = context->GetDevice()->GetLogicalDevice();
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = logicalDevice.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        accelerationStructureBuildGeometryInfo,
        triangleCount,
        mContext->GetInstance()->GetDispatcher());

    mBLASBuffer = context->GetDevice()->CreateBuffer(
        buildSizesInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::MemoryAllocateFlagBits::eDeviceAddress);

    vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = vk::AccelerationStructureCreateInfoKHR()
                                                                                 .setBuffer(mBLASBuffer->GetBufferHandle())
                                                                                 .setSize(buildSizesInfo.accelerationStructureSize)
                                                                                 .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    mBLAS = VKRT_ASSERT_VK(
        logicalDevice.createAccelerationStructureKHR(accelerationStructureCreateInfo, nullptr, mContext->GetInstance()->GetDispatcher()));

    VulkanBuffer* scratchBuffer = context->GetDevice()->CreateBuffer(
        buildSizesInfo.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::MemoryAllocateFlagBits::eDeviceAddress);

    vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
        vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setDstAccelerationStructure(mBLAS)
            .setGeometries(accelerationStructureGeometry)
            .setScratchData(scratchBuffer->GetDeviceAddress());

    vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
        vk::AccelerationStructureBuildRangeInfoKHR().setPrimitiveCount(triangleCount).setPrimitiveOffset(0).setFirstVertex(0).setTransformOffset(0);

    vk::CommandBuffer commandBuffer = context->GetDevice()->CreateCommandBuffer();
    commandBuffer.buildAccelerationStructuresKHR(
        accelerationBuildGeometryInfo,
        &accelerationStructureBuildRangeInfo,
        mContext->GetInstance()->GetDispatcher());
    context->GetDevice()->SubmitCommandAndFlush(commandBuffer);
    context->GetDevice()->DestroyCommand(commandBuffer);

    vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
        vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(mBLAS);
    mBLASAddress = logicalDevice.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo, mContext->GetInstance()->GetDispatcher());

    scratchBuffer->Release();
}

Model::~Model() {}

}  // namespace VKRT