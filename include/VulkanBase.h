#pragma once

#include "Macros.h"

#ifdef VKRT_DEBUG
#define VULKAN_HPP_ASSERT(condition) VKRT_UNUSED(condition)
#else
#define VULKAN_HPP_ASSERT(condition)
#endif
#include <vulkan/vulkan.hpp>
