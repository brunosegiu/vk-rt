#pragma once

#include "Macros.h"

#ifdef VKRT_DEBUG
#define VULKAN_HPP_ASSERT(condition) VKRT_UNUSED(condition)
#else
#define VULKAN_HPP_ASSERT(condition)
#endif

#define VKRT_ENABLE_VALIDATION

#include <vulkan/vulkan.hpp>
