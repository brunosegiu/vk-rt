#include "Model.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#include "nlohmann/json.hpp"
#include "tiny_gltf.h"

#include "DebugUtils.h"

namespace VKRT {

Model* Model::Load(Context* context, const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    if (loader.LoadBinaryFromFile(&model, &err, &warn, path)) {
        if (!model.nodes.empty()) {
            const int32_t meshIndex = model.nodes.front().mesh;
            const tinygltf::Mesh& mesh = model.meshes[meshIndex];
            const tinygltf::Primitive& primitive = mesh.primitives.front();
            const std::string positionName = "POSITION";
            const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at(positionName)];

            std::vector<glm::vec3> positions(positionAccessor.count);
            {
                const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
                const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];
                const size_t positionBufferOffset = positionBufferView.byteOffset + positionAccessor.byteOffset;
                size_t vetexStride = positionAccessor.ByteStride(positionBufferView);
                const unsigned char* positionData = &positionBuffer.data[positionBufferOffset];

                const uint32_t positionCount = static_cast<uint32_t>(positionAccessor.count);
                for (uint32_t positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
                    const float* positionDataFloat = reinterpret_cast<const float*>(&positionData[vetexStride * positionIndex]);
                    positions[positionIndex] = glm::vec3(positionDataFloat[0], -positionDataFloat[1], positionDataFloat[2]);
                }
            }

            const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
            std::vector<glm::uvec3> indices;
            {
                const uint32_t indexCount = static_cast<uint32_t>(indexAccessor.count);
                const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                const size_t indexOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
                const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexOffset]);
                indices.reserve(indexCount / 3);
                for (uint32_t i = 0; i < indexCount; i += 3) {
                    const glm::uvec3 triangle = glm::uvec3(pIndexData[i], pIndexData[i + 1], pIndexData[i + 2]);
                    indices.emplace_back(triangle);
                }
            }
            return new Model(context, positions, indices);
        }
    }
    return nullptr;
}

Model::Model(Context* context, const std::vector<glm::vec3>& vertices, const std::vector<glm::uvec3>& indices) : mContext(context) {
    mContext->AddRef();

    uint32_t triangleCount = indices.size();
    VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    {
        const size_t vertexBufferSize = vertices.size() * sizeof(glm::vec3);
        mVertexBuffer = mContext->GetDevice()->CreateBuffer(
            vertexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mVertexBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t const*>(vertices.data()), vertexBufferSize, bufferData);
        mVertexBuffer->UnmapBuffer();
    }

    {
        const size_t indexBufferSize = indices.size() * sizeof(glm::uvec3);
        mIndexBuffer = mContext->GetDevice()->CreateBuffer(
            indexBufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mIndexBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t const*>(indices.data()), indexBufferSize, bufferData);
        mIndexBuffer->UnmapBuffer();
    }

    {
        mTransformBuffer = mContext->GetDevice()->CreateBuffer(
            sizeof(vk::TransformMatrixKHR),
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* bufferData = mTransformBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t*>(&transformMatrix), sizeof(vk::TransformMatrixKHR), bufferData);
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

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = logicalDevice.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        accelerationStructureBuildGeometryInfo,
        triangleCount,
        mContext->GetDevice()->GetDispatcher());

    mBLASBuffer = mContext->GetDevice()->CreateBuffer(
        buildSizesInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::MemoryAllocateFlagBits::eDeviceAddress);

    vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = vk::AccelerationStructureCreateInfoKHR()
                                                                                 .setBuffer(mBLASBuffer->GetBufferHandle())
                                                                                 .setSize(buildSizesInfo.accelerationStructureSize)
                                                                                 .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    mBLAS = VKRT_ASSERT_VK(
        logicalDevice.createAccelerationStructureKHR(accelerationStructureCreateInfo, nullptr, mContext->GetDevice()->GetDispatcher()));

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
        vk::AccelerationStructureBuildRangeInfoKHR().setPrimitiveCount(triangleCount).setPrimitiveOffset(0).setFirstVertex(0).setTransformOffset(0);

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
    mBLASAddress = logicalDevice.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo, mContext->GetDevice()->GetDispatcher());

    scratchBuffer->Release();
}

Model::~Model() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyAccelerationStructureKHR(mBLAS, nullptr, mContext->GetDevice()->GetDispatcher());
    mBLASBuffer->Release();
    mTransformBuffer->Release();
    mIndexBuffer->Release();
    mVertexBuffer->Release();
    mContext->Release();
}

}  // namespace VKRT