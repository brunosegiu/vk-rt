#pragma once

#include <cstdint>
#include <vector>

#include "Macros.h"
#include "RefCountPtr.h"
#include "ResourceLoader.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Context;

class RayTracingPipeline : public RefCountPtr {
public:
    RayTracingPipeline(Context* context);

    const std::vector<vk::DescriptorPoolSize>& GetDescriptorSizes() const;
    const vk::DescriptorSetLayout& GetDescriptorLayout() const { return mDescriptorLayout; }
    const vk::PipelineLayout& GetPipelineLayout() const { return mLayout; }
    const vk::Pipeline& GetPipelineHandle() const { return mPipeline; }

    struct RayTracingTablesRef {
        vk::StridedDeviceAddressRegionKHR rayGen, rayHit, rayMiss, callable;
    };
    const RayTracingTablesRef& GetTablesRef() const { return mTableRef; }

    ~RayTracingPipeline();

private:
    vk::ShaderModule LoadShader(Resource::Id shaderId);

    Context* mContext;
    vk::DescriptorSetLayout mDescriptorLayout;
    std::vector<vk::DescriptorPoolSize> mDescriptorSizes;
    vk::PipelineLayout mLayout;
    vk::Pipeline mPipeline;
    std::vector<vk::ShaderModule> mShaders;

    size_t mHandleSize, mHandleSizeAligned;
    VulkanBuffer* mRayGenTable;
    VulkanBuffer* mRayHitTable;
    VulkanBuffer* mRayMissTable;
    RayTracingTablesRef mTableRef;
};
}  // namespace VKRT