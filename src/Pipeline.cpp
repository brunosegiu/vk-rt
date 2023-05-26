#include "Pipeline.h"

#include <unordered_map>

#include "Context.h"
#include "DebugUtils.h"

namespace VKRT {
Pipeline::Pipeline(
    ScopedRefPtr<Context> context,
    const std::vector<Descriptor>& descriptors,
    const std::unordered_map<RayTracingStage, Resource::Id>& shaderResourcesMap)
    : mContext(context) {
    std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings;
    std::vector<vk::DescriptorBindingFlags> bindingFlags;
    uint32_t descriptorBinding = 0;
    for (const Pipeline::Descriptor& descriptor : descriptors) {
        descriptorBindings.emplace_back(vk::DescriptorSetLayoutBinding()
                                            .setBinding(descriptorBinding)
                                            .setDescriptorType(descriptor.type)
                                            .setDescriptorCount(descriptor.count)
                                            .setStageFlags(descriptor.stageFlags));

        vk::DescriptorBindingFlags bindingFlag =
            descriptor.variableCount ? vk::DescriptorBindingFlagBits::eVariableDescriptorCount
                                     : vk::DescriptorBindingFlags{};
        bindingFlags.emplace_back(bindingFlag);
        ++descriptorBinding;
    }

    std::unordered_map<vk::DescriptorType, uint32_t> descriptorSizes;
    for (const vk::DescriptorSetLayoutBinding& binding : descriptorBindings) {
        auto it = descriptorSizes.find(binding.descriptorType);
        if (it == descriptorSizes.end()) {
            descriptorSizes[binding.descriptorType] = binding.descriptorCount;
        } else {
            descriptorSizes[binding.descriptorType] += binding.descriptorCount;
        }
    }

    mDescriptorSizes = std::vector<vk::DescriptorPoolSize>();
    for (auto descriptorSize : descriptorSizes) {
        mDescriptorSizes.emplace_back(descriptorSize.first, descriptorSize.second);
    }

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::DescriptorSetLayoutBindingFlagsCreateInfo layoutFlagsCreateInfo =
        vk::DescriptorSetLayoutBindingFlagsCreateInfo().setBindingFlags(bindingFlags);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
        vk::DescriptorSetLayoutCreateInfo()
            .setBindings(descriptorBindings)
            .setPNext(&layoutFlagsCreateInfo);
    mDescriptorLayout = VKRT_ASSERT_VK(logicalDevice.createDescriptorSetLayout(
        descriptorSetLayoutCreateInfo,
        nullptr,
        mContext->GetDevice()->GetDispatcher()));

    mShaders = std::unordered_map<RayTracingStage, vk::ShaderModule>{};
    for (const auto& entry : shaderResourcesMap) {
        mShaders.emplace(entry.first, LoadShader(entry.second));
    }

    static const std::unordered_map<RayTracingStage, vk::ShaderStageFlagBits> rayTracingStageFlags{
        {RayTracingStage::Generate, vk::ShaderStageFlagBits::eRaygenKHR},
        {RayTracingStage::Hit, vk::ShaderStageFlagBits::eClosestHitKHR},
        {RayTracingStage::Miss, vk::ShaderStageFlagBits::eMissKHR},
        {RayTracingStage::ShadowMiss, vk::ShaderStageFlagBits::eMissKHR},
    };

    std::array<RayTracingStage, 4> stageOrder{
        RayTracingStage::Generate,
        RayTracingStage::Hit,
        RayTracingStage::Miss,
        RayTracingStage::ShadowMiss,
    };

    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> rayTracingGroupCreateInfos;
    uint32_t shaderIndex = 0;
    for (const RayTracingStage stage : stageOrder) {
        if (mShaders.find(stage) != mShaders.end()) {
            stageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo()
                                           .setPName("main")
                                           .setModule(mShaders.at(stage))
                                           .setStage(rayTracingStageFlags.at(stage)));
            vk::RayTracingShaderGroupCreateInfoKHR groupCreateInfo =
                vk::RayTracingShaderGroupCreateInfoKHR()
                    .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                    .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                    .setIntersectionShader(VK_SHADER_UNUSED_KHR)
                    .setGeneralShader(VK_SHADER_UNUSED_KHR);
            if (stage == RayTracingStage::Hit) {
                groupCreateInfo.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
                groupCreateInfo.setClosestHitShader(shaderIndex);
            } else {
                groupCreateInfo.setGeneralShader(shaderIndex);
                groupCreateInfo.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
            }
            rayTracingGroupCreateInfos.push_back(groupCreateInfo);
            ++shaderIndex;
        }
    }

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

    uint32_t missTableCount = 0;
    if (mShaders.find(RayTracingStage::Miss) != mShaders.end()) {
        ++missTableCount;
    }
    if (mShaders.find(RayTracingStage::ShadowMiss) != mShaders.end()) {
        ++missTableCount;
    }

    {
        mRayMissTable = mContext->GetDevice()->CreateBuffer(
            mHandleSize * missTableCount,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayMissTableData = mRayMissTable->MapBuffer();
        std::copy_n(
            shaderHandleStorage.begin() + mHandleSizeAligned * missTableCount,
            mHandleSize * missTableCount,
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
                       .setSize(mHandleSizeAligned * missTableCount)
                       .setStride(mHandleSizeAligned),
        .callable = vk::StridedDeviceAddressRegionKHR()};
}

const std::vector<vk::DescriptorPoolSize>& Pipeline::GetDescriptorSizes() const {
    return mDescriptorSizes;
}

vk::ShaderModule Pipeline::LoadShader(Resource::Id shaderId) {
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

Pipeline::~Pipeline() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    for (auto& entry : mShaders) {
        logicalDevice.destroyShaderModule(entry.second);
    }
    logicalDevice.destroyDescriptorSetLayout(mDescriptorLayout);
    logicalDevice.destroyPipeline(mPipeline);
    logicalDevice.destroyPipelineLayout(mLayout);
}
}  // namespace VKRT