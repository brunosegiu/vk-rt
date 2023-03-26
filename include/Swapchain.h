#pragma once
#include <string>
#include <vector>

#include "RefCountPtr.h"
#include "Result.h"
#include "VulkanBase.h"

namespace VKRT {

class Context;

class Swapchain : public RefCountPtr {
public:
    Swapchain(Context* context);

    vk::Format GetFormat() { return mFormat; }
    vk::Extent2D GetExtent() { return mExtent; }

    void AcquireNextImage();

    ~Swapchain();

private:
    Context* mContext;
    vk::SurfaceKHR mSurface;
    vk::SwapchainKHR mSwapchainHandle;
    vk::Format mFormat;
    vk::Extent2D mExtent;
    std::vector<vk::Image> mImages;
    std::vector<vk::ImageView> mImageViews;
};
}  // namespace VKRT
