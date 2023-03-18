#pragma once

#include "Macros.h"

#ifdef VKRT_DEBUG
#define VULKAN_HPP_ASSERT(condition) VKRT_UNUSED(condition)
#else
#define VULKAN_HPP_ASSERT(condition)
#endif

#ifdef VKRT_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.hpp>
