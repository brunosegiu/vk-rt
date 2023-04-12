#include "Mesh.h"

#include "DebugUtils.h"
#include "Material.h"
#include "Texture.h"

namespace VKRT {

Mesh::Mesh(
    Context* context,
    const std::vector<Vertex>& vertices,
    const std::vector<glm::uvec3>& indices,
    Material* material)
    : mContext(context), mMaterial(material) {
    mContext->AddRef();
    mMaterial->AddRef();

    uint32_t triangleCount = indices.size();
    VkTransformMatrixKHR transformMatrix =
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    {
        const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
        mVertexBuffer = mContext->GetDevice()->CreateBuffer(
            vertexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mVertexBuffer->MapBuffer();
        std::copy_n(
            reinterpret_cast<uint8_t const*>(vertices.data()),
            vertexBufferSize,
            bufferData);
        mVertexBuffer->UnmapBuffer();
    }

    {
        const size_t indexBufferSize = indices.size() * sizeof(glm::uvec3);
        mIndexBuffer = mContext->GetDevice()->CreateBuffer(
            indexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mIndexBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t const*>(indices.data()), indexBufferSize, bufferData);
        mIndexBuffer->UnmapBuffer();
    }

    {
        mTransformBuffer = mContext->GetDevice()->CreateBuffer(
            sizeof(vk::TransformMatrixKHR),
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mTransformBuffer->MapBuffer();
        std::copy_n(
            reinterpret_cast<uint8_t*>(&transformMatrix),
            sizeof(vk::TransformMatrixKHR),
            bufferData);
        mTransformBuffer->UnmapBuffer();
    }

    vk::AccelerationStructureGeometryTrianglesDataKHR triangleData =
        vk::AccelerationStructureGeometryTrianglesDataKHR()
            .setVertexFormat(vk::Format::eR32G32B32A32Sfloat)
            .setVertexData(mVertexBuffer->GetDeviceAddress())
            .setMaxVertex(vertices.size())
            .setVertexStride(sizeof(Vertex))
            .setIndexType(vk::IndexType::eUint32)
            .setIndexData(mIndexBuffer->GetDeviceAddress())
            .setTransformData(mTransformBuffer->GetDeviceAddress());

    vk::AccelerationStructureGeometryKHR accelerationStructureGeometry =
        vk::AccelerationStructureGeometryKHR()
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setGeometry(triangleData);

    vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
        vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(accelerationStructureGeometry);

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        logicalDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            accelerationStructureBuildGeometryInfo,
            triangleCount,
            mContext->GetDevice()->GetDispatcher());

    mBLASBuffer = mContext->GetDevice()->CreateBuffer(
        buildSizesInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::MemoryAllocateFlagBits::eDeviceAddress);

    vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
        vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(mBLASBuffer->GetBufferHandle())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    mBLAS = VKRT_ASSERT_VK(logicalDevice.createAccelerationStructureKHR(
        accelerationStructureCreateInfo,
        nullptr,
        mContext->GetDevice()->GetDispatcher()));

    VulkanBuffer* scratchBuffer = mContext->GetDevice()->CreateBuffer(
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
        vk::AccelerationStructureBuildRangeInfoKHR()
            .setPrimitiveCount(triangleCount)
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);

    vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
    VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));
    commandBuffer.buildAccelerationStructuresKHR(
        accelerationBuildGeometryInfo,
        &accelerationStructureBuildRangeInfo,
        mContext->GetDevice()->GetDispatcher());
    VKRT_ASSERT_VK(commandBuffer.end());
    mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
    mContext->GetDevice()->DestroyCommand(commandBuffer);

    vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
        vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(mBLAS);
    mBLASAddress = logicalDevice.getAccelerationStructureAddressKHR(
        accelerationDeviceAddressInfo,
        mContext->GetDevice()->GetDispatcher());

    scratchBuffer->Release();
}

Mesh::Description Mesh::GetDescription() const {
    return Mesh::Description{
        .vertexBufferAddress = mVertexBuffer->GetDeviceAddress(),
        .indexBufferAddress = mIndexBuffer->GetDeviceAddress()};
}

Mesh::~Mesh() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyAccelerationStructureKHR(
        mBLAS,
        nullptr,
        mContext->GetDevice()->GetDispatcher());
    mBLASBuffer->Release();
    mTransformBuffer->Release();
    mIndexBuffer->Release();
    mVertexBuffer->Release();
    mContext->Release();
}

}  // namespace VKRT