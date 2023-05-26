#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Macros.h"
#include "RefCountPtr.h"
#include "ResourceLoader.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Context;

enum class RayTracingStage { Generate = 0, Hit, Miss, ShadowMiss };

class Pipeline : public RefCountPtr {
public:
    struct Descriptor {
        vk::DescriptorType type;
        vk::ShaderStageFlags stageFlags;
        uint32_t count = 1;
        bool variableCount = false;
    };
    Pipeline(
        ScopedRefPtr<Context> context,
        const std::vector<Descriptor>& descriptors,
        const std::unordered_map<RayTracingStage, Resource::Id>& shaderResourcesMap);

    const std::vector<vk::DescriptorPoolSize>& GetDescriptorSizes() const;
    const vk::DescriptorSetLayout& GetDescriptorLayout() const { return mDescriptorLayout; }
    const vk::PipelineLayout& GetPipelineLayout() const { return mLayout; }
    const vk::Pipeline& GetPipelineHandle() const { return mPipeline; }

    struct RayTracingTablesRef {
        vk::StridedDeviceAddressRegionKHR rayGen, rayHit, rayMiss, callable;
    };
    const RayTracingTablesRef& GetTablesRef() const { return mTableRef; }

    ~Pipeline();

private:
    vk::ShaderModule LoadShader(Resource::Id shaderId);

    ScopedRefPtr<Context> mContext;
    vk::DescriptorSetLayout mDescriptorLayout;
    std::vector<vk::DescriptorPoolSize> mDescriptorSizes;
    vk::PipelineLayout mLayout;
    vk::Pipeline mPipeline;
    std::unordered_map<RayTracingStage, vk::ShaderModule> mShaders;

    size_t mHandleSize, mHandleSizeAligned;
    ScopedRefPtr<VulkanBuffer> mRayGenTable;
    ScopedRefPtr<VulkanBuffer> mRayHitTable;
    ScopedRefPtr<VulkanBuffer> mRayMissTable;
    RayTracingTablesRef mTableRef;
};
}  // namespace VKRT