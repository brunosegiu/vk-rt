#include "Instance.h"

#include <GLFW/glfw3.h>

#include "DebugUtils.h"
#include "Device.h"

namespace VKRT {

#if defined(VKRT_ENABLE_VALIDATION)
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*) {
    VKRT_LOG(callbackData->pMessage);
    VKRT_ASSERT_MSG(
        messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT &&
            messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        callbackData->pMessage);
    return VK_FALSE;
}
#endif

const std::vector<const char*> Instance::sRequiredDeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};

ResultValue<ScopedRefPtr<Instance>> Instance::Create(ScopedRefPtr<Window> window) {
    const vk::ApplicationInfo appInfo = vk::ApplicationInfo()
                                            .setPApplicationName("VKRT")
                                            .setApplicationVersion(VK_MAKE_VERSION(0, 0, 1))
                                            .setPEngineName("VKRT")
                                            .setEngineVersion(VK_MAKE_VERSION(0, 0, 1))
                                            .setApiVersion(VK_API_VERSION_1_2);

    std::vector<const char*> layersToEnable {
#if defined(VKRT_ENABLE_VALIDATION)
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    std::vector<const char*> extensionsToEnable{};

    std::vector<std::string> requiredWindowExtensions = window->GetRequiredVulkanExtensions();
    for (const std::string& requiredWindowExtension : requiredWindowExtensions) {
        extensionsToEnable.push_back(requiredWindowExtension.c_str());
    }

#if defined(VKRT_ENABLE_VALIDATION)
    {
        const std::vector<const char*> debugExtensions{
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
        extensionsToEnable.insert(
            extensionsToEnable.end(),
            debugExtensions.begin(),
            debugExtensions.end());
    }
#endif

    std::vector<vk::ExtensionProperties> supportedExtensions =
        VKRT_ASSERT_VK(vk::enumerateInstanceExtensionProperties());

    auto supportedLayers = VKRT_ASSERT_VK(vk::enumerateInstanceLayerProperties());

    bool allExtensionsSupported = true;
    for (const char* extensionName : extensionsToEnable) {
        const bool isExtensionSupported =
            std::find_if(
                supportedExtensions.begin(),
                supportedExtensions.end(),
                [&extensionName](const vk::ExtensionProperties& presentExtension) {
                    return std::string(extensionName) ==
                           std::string(presentExtension.extensionName.data());
                }) != supportedExtensions.end();
        allExtensionsSupported = allExtensionsSupported && isExtensionSupported;
    }
    if (!allExtensionsSupported) {
        return {Result::DriverNotFoundError, nullptr};
    }

    const vk::InstanceCreateInfo instanceInfo = vk::InstanceCreateInfo()
                                                    .setPApplicationInfo(&appInfo)
                                                    .setPEnabledExtensionNames(extensionsToEnable)
                                                    .setPEnabledLayerNames(layersToEnable);

    auto [instanceResult, instanceHandle] = vk::createInstance(instanceInfo);
    if (instanceResult == vk::Result::eSuccess) {
        return {Result::Success, new Instance(instanceHandle)};
    }
    return {Result::DriverNotFoundError, nullptr};
}

Instance::Instance(const vk::Instance& instance) : mInstanceHandle(instance) {
    mDynamicDispatcher = vk::DispatchLoaderDynamic(mInstanceHandle, vkGetInstanceProcAddr);
    mDynamicDispatcher.init(mInstanceHandle, vkGetInstanceProcAddr);

#if defined(VKRT_ENABLE_VALIDATION)
    const vk::DebugUtilsMessengerCreateInfoEXT debugCallbackCreateInfo =
        vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(vkDebugCallback);

    mDebugMessenger = VKRT_ASSERT_VK(mInstanceHandle.createDebugUtilsMessengerEXT(
        debugCallbackCreateInfo,
        nullptr,
        mDynamicDispatcher));
#endif
}

ResultValue<vk::PhysicalDevice> Instance::FindSuitablePhysicalDevice(
    const vk::SurfaceKHR& surface) {
    const std::vector<vk::PhysicalDevice> physicalDevices =
        VKRT_ASSERT_VK(mInstanceHandle.enumeratePhysicalDevices());
    vk::PhysicalDevice chosenDevice = nullptr;
    uint32_t chosenDeviceScore = 0;
    for (const vk::PhysicalDevice& physicalDevice : physicalDevices) {
        const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        uint32_t currentDeviceScore = 0;
        {
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                currentDeviceScore += 1000;
            }
            if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                currentDeviceScore += 100;
            }

            const std::vector<vk::QueueFamilyProperties> queueFamiliesProperties =
                physicalDevice.getQueueFamilyProperties();
            bool hasGraphicsQueue = false;
            uint32_t queueFamilyIndex = 0;
            while (queueFamilyIndex < queueFamiliesProperties.size() && !hasGraphicsQueue) {
                const vk::QueueFamilyProperties& properties =
                    queueFamiliesProperties[queueFamilyIndex];
                const bool isGraphicsQueue =
                    static_cast<bool>(properties.queueFlags & vk::QueueFlagBits::eGraphics);
                if (VKRT_ASSERT_VK(
                        physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface))) {
                    hasGraphicsQueue = true;
                } else {
                    ++queueFamilyIndex;
                }
            }
            if (!hasGraphicsQueue) {
                currentDeviceScore = 0;
            }
        }

        std::vector<vk::ExtensionProperties> deviceExtensions =
            VKRT_ASSERT_VK(physicalDevice.enumerateDeviceExtensionProperties());
        bool allExtensionsSupported = true;
        for (const char* extensionName : sRequiredDeviceExtensions) {
            const bool isExtensionSupported =
                std::find_if(
                    deviceExtensions.begin(),
                    deviceExtensions.end(),
                    [&extensionName](const vk::ExtensionProperties& presentExtension) {
                        return std::string(extensionName) ==
                               std::string(presentExtension.extensionName.data());
                    }) != deviceExtensions.end();
            if (!isExtensionSupported) {
                VKRT_LOG("No extension " << extensionName);
            }
            allExtensionsSupported = allExtensionsSupported && isExtensionSupported;
        }
        if (!allExtensionsSupported) {
            currentDeviceScore = 0;
        }

        const vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
        if (!deviceFeatures.shaderInt64) {
            currentDeviceScore = 0;
        }

        vk::PhysicalDeviceVulkan11Features physicalDeviceFeatures1_1{};
        vk::PhysicalDeviceVulkan12Features physicalDeviceFeatures1_2{};
        physicalDeviceFeatures1_1.setPNext(&physicalDeviceFeatures1_2);
        vk::PhysicalDeviceFeatures2 physicalDeviceFeatures =
            vk::PhysicalDeviceFeatures2().setPNext(&physicalDeviceFeatures1_1);
        physicalDevice.getFeatures2(&physicalDeviceFeatures);

        if (!physicalDeviceFeatures1_2.scalarBlockLayout ||
            !physicalDeviceFeatures1_2.descriptorIndexing ||
            !physicalDeviceFeatures1_2.runtimeDescriptorArray ||
            !physicalDeviceFeatures1_2.descriptorBindingVariableDescriptorCount) {
            currentDeviceScore = 0;
        }

        if (chosenDeviceScore < currentDeviceScore) {
            chosenDevice = physicalDevice;
            chosenDeviceScore = currentDeviceScore;
        }
    }
    return {chosenDeviceScore > 0 ? Result::Success : Result::NoSuitableDeviceError, chosenDevice};
}

vk::SurfaceKHR Instance::CreateSurface(ScopedRefPtr<Window> window) {
    VkSurfaceKHR surface;
    VKRT_ASSERT_VK(vk::Result(
        glfwCreateWindowSurface(mInstanceHandle, window->GetNativeHandle(), nullptr, &surface)));
    return surface;
}

void Instance::DestroySurface(vk::SurfaceKHR surface) {
    mInstanceHandle.destroySurfaceKHR(surface);
}

Instance::~Instance() {
#if defined(VKRT_ENABLE_VALIDATION)
    mInstanceHandle.destroyDebugUtilsMessengerEXT(mDebugMessenger, nullptr, mDynamicDispatcher);
#endif
    mInstanceHandle.destroy();
}

}  // namespace VKRT
