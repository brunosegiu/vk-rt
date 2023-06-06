#pragma once

#include "glm/glm.hpp"

#include "Context.h"
#include "RefCountPtr.h"
#include "Texture.h"
#include "VulkanBuffer.h"

namespace VKRT {

class ProbeGrid : public RefCountPtr {
public:
    ProbeGrid(ScopedRefPtr<Context> context);

    struct UniformData {
        glm::vec3 origin;
        glm::vec3 size;
        glm::uvec3 dimensions;
        uint32_t resolution;
    };

    const ScopedRefPtr<Texture>& GetTexture() { return mProbesTexture; }
    const ScopedRefPtr<VulkanBuffer>& GetDescriptionBuffer() { return mProbeGridBuffer; }

    glm::uvec3 GetDispatchDimensions();

    void UpdateData();

    virtual ~ProbeGrid();

private:
    ScopedRefPtr<Context> mContext;
    ScopedRefPtr<Texture> mProbesTexture;
    ScopedRefPtr<VulkanBuffer> mProbeGridBuffer;
    glm::vec3 mOrigin;
    glm::vec3 mSize;
    glm::uvec3 mDimensions;
    uint32_t mResolution;
};

}  // namespace VKRT