#include "Scene.h"

#include "DebugUtils.h"

#undef MemoryBarrier

namespace VKRT {

Scene::Scene(ScopedRefPtr<Context> context)
    : mContext(context), mObjects(), mInstanceBuffer(nullptr), mTLASBuffer(nullptr) {}

void Scene::AddObject(ScopedRefPtr<Object> object) {
    if (object != nullptr) {
        mObjects.emplace_back(object);
    }
}

void Scene::AddLight(ScopedRefPtr<Light> light) {
    if (light != nullptr) {
        mLights.emplace_back(light);
    }
}

std::vector<Mesh::Description> Scene::GetDescriptions() {
    std::vector<Mesh::Description> descriptions;
    for (const ScopedRefPtr<Object>& object : mObjects) {
        std::vector<Mesh::Description> modelDescriptions = object->GetModel()->GetDescriptions();
        descriptions.insert(descriptions.end(), modelDescriptions.begin(), modelDescriptions.end());
    }
    return descriptions;
}

std::vector<Light::Proxy> Scene::GetLightDescriptions() {
    std::vector<Light::Proxy> proxies;
    for (const ScopedRefPtr<Light>& light : mLights) {
        proxies.push_back(light->GetProxy());
    }
    return proxies;
}

Scene::SceneMaterials Scene::GetMaterialProxies() {
    // Gather textures first
    std::vector<std::pair<ScopedRefPtr<Texture>, int32_t>> textureIndices;
    int32_t currentTextureIndex = 0;
    for (const ScopedRefPtr<Object>& object : mObjects) {
        for (const ScopedRefPtr<Mesh>& mesh : object->GetModel()->GetMeshes()) {
            const Material* material = mesh->GetMaterial();
            std::vector<ScopedRefPtr<Texture>> meshTextures{
                material->GetAlbedoTexture(),
                material->GetRoughnessTexture()};
            for (const ScopedRefPtr<Texture>& texture : meshTextures) {
                if (texture != nullptr) {
                    auto it = std::find_if(
                        textureIndices.begin(),
                        textureIndices.end(),
                        [&texture](const auto& entry) { return entry.first == texture; });

                    if (it == textureIndices.end()) {
                        textureIndices.emplace_back(texture, currentTextureIndex);
                        ++currentTextureIndex;
                    }
                }
            }
        }
    }

    // Gather materials and generate texture indices if applicable
    std::vector<MaterialProxy> materials;
    for (const ScopedRefPtr<Object>& object : mObjects) {
        for (const ScopedRefPtr<Mesh>& mesh : object->GetModel()->GetMeshes()) {
            const ScopedRefPtr<Material> material = mesh->GetMaterial();
            MaterialProxy proxy{
                .albedo = material->GetAlbedo(),
                .roughness = material->GetRoughness(),
                .metallic = material->GetMetallic(),
                .indexOfRefraction = material->GetIndexOfRefraction(),
                .albedoTextureIndex = -1,
                .roughnessTextureIndex = -1,
            };
            {
                const ScopedRefPtr<Texture> albedoTexture = material->GetAlbedoTexture();
                auto albedoIt = std::find_if(
                    textureIndices.begin(),
                    textureIndices.end(),
                    [&albedoTexture](const auto& entry) { return entry.first == albedoTexture; });
                if (albedoIt != textureIndices.end()) {
                    proxy.albedoTextureIndex = albedoIt->second;
                }
            }
            {
                const ScopedRefPtr<Texture> roughnessTexture = material->GetRoughnessTexture();
                auto roughnessIt = std::find_if(
                    textureIndices.begin(),
                    textureIndices.end(),
                    [&roughnessTexture](const auto& entry) {
                        return entry.first == roughnessTexture;
                    });
                if (roughnessIt != textureIndices.end()) {
                    proxy.roughnessTextureIndex = roughnessIt->second;
                }
            }
            materials.push_back(proxy);
        }
    }

    std::vector<ScopedRefPtr<Texture>> textures;
    for (const auto& entry : textureIndices) {
        textures.push_back(entry.first);
    }

    Scene::SceneMaterials sceneMaterials{
        .materials = materials,
        .textures = textures,
    };
    return sceneMaterials;
}

void Scene::Update(vk::CommandBuffer& commandBuffer) {
    bool isUpdate = mTLAS;
    if (!mObjects.empty()) {
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        uint32_t index = 0;
        for (const Object* object : mObjects) {
            const glm::mat4& transform = glm::transpose(object->GetTransform());
            VkTransformMatrixKHR transformMatrix =
                *(reinterpret_cast<const VkTransformMatrixKHR*>(&transform));
            for (const Mesh* mesh : object->GetModel()->GetMeshes()) {
                const bool isRefractive = mesh->GetMaterial()->GetIndexOfRefraction() > 0.0f;
                instances.emplace_back(
                    vk::AccelerationStructureInstanceKHR()
                        .setTransform(transformMatrix)
                        .setInstanceCustomIndex(index)
                        .setAccelerationStructureReference(mesh->GetBLASAddress())
                        .setMask(isRefractive ? Material::RefractiveMask : Material::OpaqueMask)
                        .setInstanceShaderBindingTableRecordOffset(0)
                        .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable));
                ++index;
            }
        }

        const size_t instanceDataSize =
            instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        mInstanceBuffer = mContext->GetDevice()->CreateBuffer(
            instanceDataSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* instanceData = mInstanceBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t*>(instances.data()), instanceDataSize, instanceData);
        mInstanceBuffer->UnmapBuffer();
        const vk::DeviceAddress instanceBufferAddress = mInstanceBuffer->GetDeviceAddress();

        vk::AccelerationStructureGeometryInstancesDataKHR instancesData =
            vk::AccelerationStructureGeometryInstancesDataKHR().setArrayOfPointers(false).setData(
                instanceBufferAddress);
        vk::AccelerationStructureGeometryKHR accelerationStructureGeometry =
            vk::AccelerationStructureGeometryKHR()
                .setGeometryType(vk::GeometryTypeKHR::eInstances)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
                .setGeometry(instancesData);

        vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
            vk::AccelerationStructureBuildGeometryInfoKHR()
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(
                    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                    vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
                .setMode(
                    isUpdate ? vk::BuildAccelerationStructureModeKHR::eUpdate
                             : vk::BuildAccelerationStructureModeKHR::eBuild)
                .setGeometries(accelerationStructureGeometry);

        uint32_t instanceCount = static_cast<uint32_t>(instances.size());
        vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
        vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
            logicalDevice.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                accelerationStructureBuildGeometryInfo,
                instanceCount,
                mContext->GetDevice()->GetDispatcher());

        if (!isUpdate) {
            mTLASBuffer = mContext->GetDevice()->CreateBuffer(
                buildSizesInfo.accelerationStructureSize,
                vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                vk::MemoryAllocateFlagBits::eDeviceAddress);

            vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
                vk::AccelerationStructureCreateInfoKHR()
                    .setBuffer(mTLASBuffer->GetBufferHandle())
                    .setSize(buildSizesInfo.accelerationStructureSize)
                    .setType(vk::AccelerationStructureTypeKHR::eTopLevel);
            mTLAS = VKRT_ASSERT_VK(logicalDevice.createAccelerationStructureKHR(
                accelerationStructureCreateInfo,
                nullptr,
                mContext->GetDevice()->GetDispatcher()));
        }

        mScratchBuffer = mContext->GetDevice()->CreateBuffer(
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
                .setScratchData(mScratchBuffer->GetDeviceAddress())
                .setSrcAccelerationStructure(isUpdate ? mTLAS : nullptr);

        vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
            vk::AccelerationStructureBuildRangeInfoKHR()
                .setPrimitiveCount(instanceCount)
                .setPrimitiveOffset(0)
                .setFirstVertex(0)
                .setTransformOffset(0);

        commandBuffer.buildAccelerationStructuresKHR(
            accelerationBuildGeometryInfo,
            &accelerationStructureBuildRangeInfo,
            mContext->GetDevice()->GetDispatcher());

        vk::MemoryBarrier barrier =
            vk::MemoryBarrier()
                .setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                .setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {},
            barrier,
            {},
            {});

        if (!isUpdate) {
            vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
                vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(mTLAS);
            mTLASAddress = logicalDevice.getAccelerationStructureAddressKHR(
                accelerationDeviceAddressInfo,
                mContext->GetDevice()->GetDispatcher());
        }
    }
}

Scene::~Scene() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    if (mTLAS) {
        logicalDevice.destroyAccelerationStructureKHR(
            mTLAS,
            nullptr,
            mContext->GetDevice()->GetDispatcher());
    }
}

}  // namespace VKRT