#include "ProbeGrid.h"

namespace VKRT {
ProbeGrid::ProbeGrid(ScopedRefPtr<Context> context)
    : mContext(context),
      mProbesTexture(nullptr),
      mProbeGridBuffer(nullptr),
      mOrigin(),
      mSize(1, 1, 1),
      mDimensions(1, 1, 1),
      mResolution(1024) {
    const uint32_t horizontalResolution = mResolution * mDimensions.x;
    const uint32_t verticalResolution = mResolution * mDimensions.y;
    const uint32_t layerCount = mDimensions.z;

    mProbesTexture = new Texture(
        mContext,
        horizontalResolution,
        verticalResolution,
        layerCount,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);

    mProbeGridBuffer = mContext->GetDevice()->CreateBuffer(
        sizeof(ProbeGrid::UniformData),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void ProbeGrid::UpdateData() {
    uint8_t* buffer = mProbeGridBuffer->MapBuffer();
    UniformData data{
        .origin = mOrigin,
        .size = mSize,
        .dimensions = mDimensions,
        .resolution = mResolution,
    };
    std::copy_n(reinterpret_cast<uint8_t*>(&data), sizeof(UniformData), buffer);
    mProbeGridBuffer->UnmapBuffer();
}

glm::uvec3 ProbeGrid::GetDispatchDimensions() {
    return glm::uvec3(mResolution * mDimensions.x, mResolution * mDimensions.y, mDimensions.z);
}

ProbeGrid::~ProbeGrid() {}

}  // namespace VKRT