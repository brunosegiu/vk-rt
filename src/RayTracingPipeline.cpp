#include "RayTracingPipeline.h"

#include "Context.h"
#include "DebugUtils.h"

enum class RayTracingStage { Generate = 0, Hit, Miss };

namespace VKRT {
RayTracingPipeline::RayTracingPipeline(Context* context) : mContext(context) {
    mContext->AddRef();

    mShaders = std::vector<vk::ShaderModule>{
        LoadShader(Resource::Id::GenShader),
        LoadShader(Resource::Id::HitShader),
        LoadShader(Resource::Id::MissShader),
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
            .setStage(vk::ShaderStageFlagBits::eMissKHR)};

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
    };

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::PipelineLayoutCreateInfo layoutCreateInfo = vk::PipelineLayoutCreateInfo();
    mLayout = VKRT_ASSERT_VK(logicalDevice.createPipelineLayout(layoutCreateInfo));

    vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = vk::RayTracingPipelineCreateInfoKHR()
                                                                           .setStages(stageCreateInfos)
                                                                           .setGroups(rayTracingGroupCreateInfos)
                                                                           .setMaxPipelineRayRecursionDepth(1)
                                                                           .setLayout(mLayout);
    mPipeline = VKRT_ASSERT_VK(logicalDevice.createRayTracingPipelineKHR({}, {}, rayTracingPipelineCreateInfo, nullptr, mContext->GetInstance()->GetDispatcher()));
}

vk::ShaderModule RayTracingPipeline::LoadShader(Resource::Id shaderId) {
    Resource shaderResource = ResourceLoader::Load(shaderId);
    vk::ShaderModuleCreateInfo shaderCreateInfo = vk::ShaderModuleCreateInfo()
                                                      .setCodeSize(shaderResource.size * sizeof(uint8_t))
                                                      .setPCode(reinterpret_cast<const uint32_t*>(shaderResource.buffer));
    vk::ShaderModule shaderModule = VKRT_ASSERT_VK(mContext->GetDevice()->GetLogicalDevice().createShaderModule(shaderCreateInfo));
    ResourceLoader::CleanUp(shaderResource);
    return shaderModule;
}

RayTracingPipeline::~RayTracingPipeline() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    for (vk::ShaderModule& shader : mShaders) {
        logicalDevice.destroyShaderModule(shader);
    }
    logicalDevice.destroyPipeline(mPipeline);
    logicalDevice.destroyPipelineLayout(mLayout);
    mContext->Release();
}
}  // namespace VKRT