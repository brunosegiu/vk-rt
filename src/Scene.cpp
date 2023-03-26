#include "Scene.h"

#include "DebugUtils.h"

namespace VKRT {

Scene::Scene(Context* context) : mContext(context), mObjects(), mInstanceBuffer(nullptr) {
    context->AddRef();
}

void Scene::AddObject(Object* object) {}

void Scene::Commit() {
    if (!mObjects.empty()) {
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        uint32_t index = 0;
        for (Object* object : mObjects) {
            vk::TransformMatrixKHR transformMatrix = std::array<std::array<float, 4>, 3>{
                std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.0f},
                std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.0f},
                std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.0f}};
            vk::AccelerationStructureInstanceKHR accelerationStructureInstance =
                vk::AccelerationStructureInstanceKHR()
                    .setTransform(transformMatrix)
                    .setInstanceCustomIndex(index)
                    .setAccelerationStructureReference(object->GetModel()->GetBLASAddress())
                    .setMask(0xFF)
                    .setInstanceShaderBindingTableRecordOffset(0)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            ++index;
        }

        const size_t instanceDataSize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        mInstanceBuffer = mContext->GetDevice()->CreateBuffer(
            instanceDataSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uint8_t* instanceData = mInstanceBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t*>(instances.data()), instanceDataSize, instanceData);
        mInstanceBuffer->UnmapBuffer();
        const vk::DeviceAddress instanceBufferAddress = mInstanceBuffer->GetDeviceAddress();

        vk::AccelerationStructureGeometryInstancesDataKHR instancesData =
            vk::AccelerationStructureGeometryInstancesDataKHR().setArrayOfPointers(false).setData(instanceBufferAddress);
        vk::AccelerationStructureGeometryKHR accelerationStructureGeometry = vk::AccelerationStructureGeometryKHR()
                                                                                 .setGeometryType(vk::GeometryTypeKHR::eInstances)
                                                                                 .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
                                                                                 .setGeometry(instancesData);

        vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
            vk::AccelerationStructureBuildGeometryInfoKHR()
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setGeometries(accelerationStructureGeometry);

        uint32_t instanceCount = static_cast<uint32_t>(instances.size());
        vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
        vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = logicalDevice.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            accelerationStructureBuildGeometryInfo,
            instanceCount,
            mContext->GetInstance()->GetDispatcher());

        mTLASBuffer = mContext->GetDevice()->CreateBuffer(
            buildSizesInfo.accelerationStructureSize,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryAllocateFlagBits::eDeviceAddress);

        vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = vk::AccelerationStructureCreateInfoKHR()
                                                                                     .setBuffer(mTLASBuffer->GetBufferHandle())
                                                                                     .setSize(buildSizesInfo.accelerationStructureSize)
                                                                                     .setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        mTLAS = VKRT_ASSERT_VK(
            logicalDevice.createAccelerationStructureKHR(accelerationStructureCreateInfo, nullptr, mContext->GetInstance()->GetDispatcher()));

        VulkanBuffer* scratchBuffer = mContext->GetDevice()->CreateBuffer(
            buildSizesInfo.buildScratchSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryAllocateFlagBits::eDeviceAddress);

        vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
            vk::AccelerationStructureBuildGeometryInfoKHR()
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                .setDstAccelerationStructure(mTLAS)
                .setGeometries(accelerationStructureGeometry)
                .setScratchData(scratchBuffer->GetDeviceAddress());

        vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
                                                                                             .setPrimitiveCount(instanceCount)
                                                                                             .setPrimitiveOffset(0)
                                                                                             .setFirstVertex(0)
                                                                                             .setTransformOffset(0);
        vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
        commandBuffer.buildAccelerationStructuresKHR(
            accelerationBuildGeometryInfo,
            &accelerationStructureBuildRangeInfo,
            mContext->GetInstance()->GetDispatcher());
        mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
        mContext->GetDevice()->DestroyCommand(commandBuffer);

        vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
            vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(mTLAS);
        mTLASAddress = logicalDevice.getAccelerationStructureAddressKHR(accelerationDeviceAddressInfo, mContext->GetInstance()->GetDispatcher());

        scratchBuffer->Release();
    }
}

Scene::~Scene() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroyAccelerationStructureKHR(mTLAS, nullptr, mContext->GetInstance()->GetDispatcher());
    mTLASBuffer->Release();
    mInstanceBuffer->Release();
    for (Object* object : mObjects) {
        object->Release();
    }
    mContext->Release();
}

}  // namespace VKRT