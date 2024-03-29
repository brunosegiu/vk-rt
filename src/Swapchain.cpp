#include "Swapchain.h"

#include <algorithm>
#include <limits>

#include "Context.h"
#include "DebugUtils.h"
#include "Texture.h"

namespace VKRT {

Swapchain::Swapchain(ScopedRefPtr<Context> context) : mContext(context), mCurrentImageIndex(0) {
    Device::SwapchainCapabilities swapchainCapabilities =
        mContext->GetDevice()->GetSwapchainCapabilities(mContext->GetSurface());

    vk::SurfaceFormatKHR surfaceFormat(
        vk::Format::eB8G8R8A8Unorm,
        vk::ColorSpaceKHR::eSrgbNonlinear);
    const bool supportsPreferredFormat =
        std::find(
            swapchainCapabilities.supportedFormats.begin(),
            swapchainCapabilities.supportedFormats.end(),
            surfaceFormat) != swapchainCapabilities.supportedFormats.end();
    if (!supportsPreferredFormat) {
        surfaceFormat = swapchainCapabilities.supportedFormats.front();
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
    const bool supportsPreferredPresentMode =
        std::find(
            swapchainCapabilities.supportedPresentModes.begin(),
            swapchainCapabilities.supportedPresentModes.end(),
            presentMode) != swapchainCapabilities.supportedPresentModes.end();
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
            std::clamp<uint32_t>(
                windowSize.width,
                surfaceCaps.minImageExtent.width,
                surfaceCaps.maxImageExtent.width),
            std::clamp<uint32_t>(
                windowSize.height,
                surfaceCaps.minImageExtent.height,
                surfaceCaps.maxImageExtent.height));
    }

    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount != 0 && imageCount > surfaceCaps.maxImageCount) {
        imageCount = surfaceCaps.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo =
        vk::SwapchainCreateInfoKHR()
            .setSurface(mContext->GetSurface())
            .setMinImageCount(imageCount)
            .setImageFormat(surfaceFormat.format)
            .setImageColorSpace(surfaceFormat.colorSpace)
            .setImageExtent(surfaceExtent)
            .setImageArrayLayers(1)
            .setImageUsage(
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
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
    std::vector<vk::Image> images =
        VKRT_ASSERT_VK(logicalDevice.getSwapchainImagesKHR(mSwapchainHandle));

    for (vk::Image& image : images) {
        ScopedRefPtr<Texture> texture =
            new Texture(mContext, surfaceExtent.width, surfaceExtent.height, mFormat, {}, image);
        mImages.emplace_back(texture);
    }

    mPresentSemaphore = VKRT_ASSERT_VK(logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{}));
    mRenderSemaphore = VKRT_ASSERT_VK(logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{}));
}

void Swapchain::AcquireNextImage() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    mCurrentImageIndex = VKRT_ASSERT_VK(logicalDevice.acquireNextImageKHR(
        mSwapchainHandle,
        std::numeric_limits<uint64_t>::max(),
        mPresentSemaphore));
}

void Swapchain::Present() {
    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
                                         .setSwapchains(mSwapchainHandle)
                                         .setImageIndices(mCurrentImageIndex)
                                         .setWaitSemaphores(mRenderSemaphore);
    const vk::Queue& queue = mContext->GetDevice()->GetQueue();
    VKRT_ASSERT_VK(queue.presentKHR(presentInfo));
}

Swapchain::~Swapchain() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    logicalDevice.destroySemaphore(mRenderSemaphore);
    logicalDevice.destroySemaphore(mPresentSemaphore);
    mImages.clear();
    logicalDevice.destroySwapchainKHR(mSwapchainHandle);
}
}  // namespace VKRT