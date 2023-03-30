#include "Swapchain.h"

#include <algorithm>
#include <limits>

#include "Context.h"
#include "DebugUtils.h"

namespace VKRT {

Swapchain::Swapchain(Context* context) : mContext(context), mCurrentImageIndex(0) {
    mContext->AddRef();

    Device::SwapchainCapabilities swapchainCapabilities = mContext->GetDevice()->GetSwapchainCapabilities(mContext->GetSurface());

    vk::SurfaceFormatKHR surfaceFormat(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear);
    const bool supportsPreferredFormat =
        std::find(swapchainCapabilities.supportedFormats.begin(), swapchainCapabilities.supportedFormats.end(), surfaceFormat) !=
        swapchainCapabilities.supportedFormats.end();
    if (!supportsPreferredFormat) {
        surfaceFormat = swapchainCapabilities.supportedFormats.front();
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
    const bool supportsPreferredPresentMode =
        std::find(swapchainCapabilities.supportedPresentModes.begin(), swapchainCapabilities.supportedPresentModes.end(), presentMode) !=
        swapchainCapabilities.supportedPresentModes.end();
    if (!supportsPreferredPresentMode) {
        presentMode = vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D surfaceExtent;
    const vk::SurfaceCapabilitiesKHR& surfaceCaps = swapchainCapabilities.surfaceCapabilities;
    if (surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        surfaceExtent = swapchainCapabilities.surfaceCapabilities.currentExtent;
    } else {
        Window::Size2D windowSize = mContext->GetWindow()->GetSize();
        surfaceExtent = vk::Extent2D(
            std::clamp<uint32_t>(windowSize.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
            std::clamp<uint32_t>(windowSize.height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height));
    }

    uint32_t imageCount = std::clamp<uint32_t>(surfaceCaps.minImageCount + 1, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo =
        vk::SwapchainCreateInfoKHR()
            .setSurface(mContext->GetSurface())
            .setMinImageCount(imageCount)
            .setImageFormat(surfaceFormat.format)
            .setImageColorSpace(surfaceFormat.colorSpace)
            .setImageExtent(surfaceExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surfaceCaps.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode)
            .setClipped(true)
            .setOldSwapchain(nullptr);

    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    mSwapchainHandle = VKRT_ASSERT_VK(logicalDevice.createSwapchainKHR(swapchainCreateInfo));

    mFormat = surfaceFormat.format;
    mExtent = surfaceExtent;
    mImages = VKRT_ASSERT_VK(logicalDevice.getSwapchainImagesKHR(mSwapchainHandle));

    for (vk::Image& image : mImages) {
        vk::ImageViewCreateInfo imageViewCreateInfo =
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(mFormat)
                .setComponents(vk::ComponentMapping{})
                .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        vk::ImageView imageView = VKRT_ASSERT_VK(logicalDevice.createImageView(imageViewCreateInfo));
        mImageViews.emplace_back(imageView);
    }

    mPresentSemaphore = VKRT_ASSERT_VK(logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{}));
    mRenderSemaphore = VKRT_ASSERT_VK(logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{}));
}

void Swapchain::AcquireNextImage() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    mCurrentImageIndex = VKRT_ASSERT_VK(logicalDevice.acquireNextImageKHR(mSwapchainHandle, std::numeric_limits<uint64_t>::max(), mPresentSemaphore));
}

void Swapchain::Present() {
    vk::PresentInfoKHR presentInfo =
        vk::PresentInfoKHR().setSwapchains(mSwapchainHandle).setImageIndices(mCurrentImageIndex).setWaitSemaphores(mRenderSemaphore);
    const vk::Queue& queue = mContext->GetDevice()->GetQueue();
    VKRT_ASSERT_VK(queue.presentKHR(presentInfo));
}

Swapchain::~Swapchain() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroySemaphore(mRenderSemaphore);
    logicalDevice.destroySemaphore(mPresentSemaphore);
    for (vk::ImageView& imageView : mImageViews) {
        logicalDevice.destroyImageView(imageView);
    }
    logicalDevice.destroySwapchainKHR(mSwapchainHandle);
    mContext->Release();
}
}  // namespace VKRT