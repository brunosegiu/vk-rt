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

    vk::Image& GetCurrentImage() { return mImages[mCurrentImageIndex]; }

    void AcquireNextImage();
    void Present();

    vk::Semaphore& GetPresentSemaphore() { return mPresentSemaphore; }
    vk::Semaphore& GetRenderSemaphore() { return mRenderSemaphore; }

    ~Swapchain();

private:
    Context* mContext;
    vk::SwapchainKHR mSwapchainHandle;
    vk::Format mFormat;
    vk::Extent2D mExtent;
    std::vector<vk::Image> mImages;
    std::vector<vk::ImageView> mImageViews;
    vk::Semaphore mPresentSemaphore;
    vk::Semaphore mRenderSemaphore;
    uint32_t mCurrentImageIndex;
};
}  // namespace VKRT
