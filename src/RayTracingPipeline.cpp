#include "RayTracingPipeline.h"

#include <unordered_map>

#include "Context.h"
#include "DebugUtils.h"

enum class RayTracingStage { Generate = 0, Hit, Miss, ShadowMiss };

namespace VKRT {
RayTracingPipeline::RayTracingPipeline(Context* context) : mContext(context) {
    mContext->AddRef();

    vk::DescriptorSetLayoutBinding accelerationStructureLayoutBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDescriptorCount(1)
            .setStageFlags(
                vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    vk::DescriptorSetLayoutBinding resultImageLayoutBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    vk::DescriptorSetLayoutBinding cameraUniformBufferBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(2)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    vk::DescriptorSetLayoutBinding sceneUniformBufferBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(3)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    vk::DescriptorSetLayoutBinding lightMetadataUniformBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(4)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    vk::DescriptorSetLayoutBinding lightUniformBufferBinding =
        vk::DescriptorSetLayoutBinding()
            .setBinding(5)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings{
        accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
        cameraUniformBufferBinding,
        sceneUniformBufferBinding,
        lightMetadataUniformBinding,
        lightUniformBufferBinding};

    std::unordered_map<vk::DescriptorType, uint32_t> descriptorSizes;
    for (const vk::DescriptorSetLayoutBinding& binding : descriptorBindings) {
        auto it = descriptorSizes.find(binding.descriptorType);
        if (it == descriptorSizes.end()) {
            descriptorSizes[binding.descriptorType] = 1;
        } else {
            descriptorSizes[binding.descriptorType] += 1;
        }
    }

    mDescriptorSizes = std::vector<vk::DescriptorPoolSize>();
    for (auto descriptorSize : descriptorSizes) {
        mDescriptorSizes.emplace_back(descriptorSize.first, descriptorSize.second);
    }

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
        vk::DescriptorSetLayoutCreateInfo().setBindings(descriptorBindings);
    mDescriptorLayout =
        VKRT_ASSERT_VK(logicalDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo));

    mShaders = std::vector<vk::ShaderModule>{
        LoadShader(Resource::Id::GenShader),
        LoadShader(Resource::Id::HitShader),
        LoadShader(Resource::Id::MissShader),
        LoadShader(Resource::Id::ShadowMissShader),
    };

    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos{
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Generate)))
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR),
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Hit)))
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR),
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Miss)))
            .setStage(vk::ShaderStageFlagBits::eMissKHR),
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::ShadowMiss)))
            .setStage(vk::ShaderStageFlagBits::eMissKHR),
    };

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> rayTracingGroupCreateInfos{
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(static_cast<uint32_t>(RayTracingStage::Generate)),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setClosestHitShader(static_cast<uint32_t>(RayTracingStage::Hit)),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(static_cast<uint32_t>(RayTracingStage::Miss)),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(static_cast<uint32_t>(RayTracingStage::ShadowMiss)),
    };

    vk::PipelineLayoutCreateInfo layoutCreateInfo =
        vk::PipelineLayoutCreateInfo().setSetLayouts(mDescriptorLayout);
    mLayout = VKRT_ASSERT_VK(logicalDevice.createPipelineLayout(layoutCreateInfo));

    vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo =
        vk::RayTracingPipelineCreateInfoKHR()
            .setStages(stageCreateInfos)
            .setGroups(rayTracingGroupCreateInfos)
            .setMaxPipelineRayRecursionDepth(
                mContext->GetDevice()->GetRayTracingProperties().maxRayRecursionDepth)
            .setLayout(mLayout);
    mPipeline = VKRT_ASSERT_VK(logicalDevice.createRayTracingPipelineKHR(
        {},
        {},
        rayTracingPipelineCreateInfo,
        nullptr,
        mContext->GetDevice()->GetDispatcher()));

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties =
        mContext->GetDevice()->GetRayTracingProperties();
    mHandleSize = rayTracingProperties.shaderGroupHandleSize;
    const size_t handleAlignment = rayTracingProperties.shaderGroupHandleAlignment;
    mHandleSizeAligned = (mHandleSize + handleAlignment - 1) & ~(handleAlignment - 1);
    const uint32_t groupCount = static_cast<uint32_t>(rayTracingGroupCreateInfos.size());
    const size_t sbtSize = groupCount * mHandleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage =
        VKRT_ASSERT_VK(logicalDevice.getRayTracingShaderGroupHandlesKHR<uint8_t>(
            mPipeline,
            0,
            groupCount,
            sbtSize,
            mContext->GetDevice()->GetDispatcher()));

    {
        mRayGenTable = mContext->GetDevice()->CreateBuffer(
            mHandleSize,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayGenTableData = mRayGenTable->MapBuffer();
        std::copy_n(shaderHandleStorage.begin(), mHandleSize, rayGenTableData);
        mRayGenTable->UnmapBuffer();
    }

    {
        mRayHitTable = mContext->GetDevice()->CreateBuffer(
            mHandleSize,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayHitTableData = mRayHitTable->MapBuffer();
        std::copy_n(shaderHandleStorage.begin() + mHandleSizeAligned, mHandleSize, rayHitTableData);
        mRayHitTable->UnmapBuffer();
    }

    {
        mRayMissTable = mContext->GetDevice()->CreateBuffer(
            mHandleSize * 2,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayMissTableData = mRayMissTable->MapBuffer();
        std::copy_n(
            shaderHandleStorage.begin() + mHandleSizeAligned * 2,
            mHandleSize * 2,
            rayMissTableData);
        mRayMissTable->UnmapBuffer();
    }

    mTableRef = RayTracingTablesRef{
        .rayGen = vk::StridedDeviceAddressRegionKHR()
                      .setDeviceAddress(mRayGenTable->GetDeviceAddress())
                      .setSize(mHandleSizeAligned)
                      .setStride(mHandleSizeAligned),
        .rayHit = vk::StridedDeviceAddressRegionKHR()
                      .setDeviceAddress(mRayHitTable->GetDeviceAddress())
                      .setSize(mHandleSizeAligned)
                      .setStride(mHandleSizeAligned),
        .rayMiss = vk::StridedDeviceAddressRegionKHR()
                       .setDeviceAddress(mRayMissTable->GetDeviceAddress())
                       .setSize(mHandleSizeAligned * 2)
                       .setStride(mHandleSizeAligned),
        .callable = vk::StridedDeviceAddressRegionKHR()};
}

const std::vector<vk::DescriptorPoolSize>& RayTracingPipeline::GetDescriptorSizes() const {
    return mDescriptorSizes;
}

vk::ShaderModule RayTracingPipeline::LoadShader(Resource::Id shaderId) {
    Resource shaderResource = ResourceLoader::Load(shaderId);
    vk::ShaderModuleCreateInfo shaderCreateInfo =
        vk::ShaderModuleCreateInfo()
            .setCodeSize(shaderResource.size * sizeof(uint8_t))
            .setPCode(reinterpret_cast<const uint32_t*>(shaderResource.buffer));
    vk::ShaderModule shaderModule = VKRT_ASSERT_VK(
        mContext->GetDevice()->GetLogicalDevice().createShaderModule(shaderCreateInfo));
    ResourceLoader::CleanUp(shaderResource);
    return shaderModule;
}

RayTracingPipeline::~RayTracingPipeline() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    mRayGenTable->Release();
    mRayHitTable->Release();
    mRayMissTable->Release();
    for (vk::ShaderModule& shader : mShaders) {
        logicalDevice.destroyShaderModule(shader);
    }
    logicalDevice.destroyPipeline(mPipeline);
    logicalDevice.destroyPipelineLayout(mLayout);
    mContext->Release();
}
}  // namespace VKRT