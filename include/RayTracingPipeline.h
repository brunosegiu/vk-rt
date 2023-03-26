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

    ~RayTracingPipeline();

private:
    vk::ShaderModule LoadShader(Resource::Id shaderId);

    Context* mContext;
    vk::DescriptorSetLayout mDescriptorLayout;
    vk::PipelineLayout mLayout;
    vk::Pipeline mPipeline;
    std::vector<vk::ShaderModule> mShaders;

    VulkanBuffer* mRayGenTable;
    VulkanBuffer* mRayHitTable;
    VulkanBuffer* mRayMissTable;
};
}  // namespace VKRT