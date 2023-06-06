#include "ProbeGrid.h"

namespace VKRT {
ProbeGrid::ProbeGrid(ScopedRefPtr<Context> context)
    : mContext(context),
      mProbesTexture(nullptr),
      mProbeGridBuffer(nullptr),
      mOrigin(0.0f, 20.0f, 0.0f),
      mSize(40.0f, 40.0f, 40.0f),
      mDimensions(8, 8, 8),
      mResolution(64) {
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
        .origin = mOrigin - mSize / 2.0f,
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