#pragma once

#include <cstdint>
#include <vector>

#include "Macros.h"
#include "RefCountPtr.h"
#include "ResourceLoader.h"
#include "VulkanBase.h"

namespace VKRT {

class Context;

class RayTracingPipeline : public RefCountPtr {
public:
    RayTracingPipeline(Context* context);

    ~RayTracingPipeline();

private:
    vk::ShaderModule LoadShader(Resource::Id shaderId);

    Context* mContext;
    vk::PipelineLayout mLayout;
    vk::Pipeline mPipeline;
    std::vector<vk::ShaderModule> mShaders;
};
}  // namespace VKRT