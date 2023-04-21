#pragma once

#include <iostream>
#include <sstream>
#include <string>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Macros.h"
#include "VulkanBase.h"

#ifdef VKRT_PLATFORM_WINDOWS
#include <Windows.h>
#include <debugapi.h>
#endif

#if defined(VKRT_PLATFORM_WINDOWS)
#define VKRT_DEBUG_BREAK() __debugbreak()
#elif defined(VKRT_PLATFORM_LINUX)
#define VKRT_DEBUG_BREAK() __builtin_trap()
#endif

#if defined(VKRT_PLATFORM_WINDOWS)
#define VKRT_LOG(message)                      \
    {                                          \
        std::ostringstream os_;                \
        os_ << message << std::endl;           \
        OutputDebugStringA(os_.str().c_str()); \
    }
#elif defined(VKRT_PLATFORM_LINUX)
#define VKRT_LOG(message) \
    { std::cout << message << std::endl; }
#endif

#ifdef VKRT_DEBUG
#define VKRT_ASSERT_MSG(condition, message)                                                     \
    {                                                                                           \
        if (!(condition)) {                                                                     \
            VKRT_LOG(                                                                           \
                "In file: " << __FILE__ << ", line: " << __LINE__ << " of function: "           \
                            << __FUNCTION__ << "Condition failed : " << #condition << std::endl \
                            << message << std::endl);                                           \
            VKRT_DEBUG_BREAK();                                                                 \
        }                                                                                       \
    }
#else
#if defined(VKRT_PLATFORM_WINDOWS)
#define VKRT_ASSERT_MSG(condition, message)                                                      \
    {                                                                                            \
        if (!(condition)) {                                                                      \
            std::ostringstream os_;                                                              \
            os_ << "In file: " << __FILE__ << ", line: " << __LINE__                             \
                << " of function: " << __FUNCTION__ << "Condition failed : " << #condition       \
                << std::endl                                                                     \
                << message << std::endl;                                                         \
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", os_.str().c_str(), nullptr); \
        }                                                                                        \
    }
#else
#define VKRT_ASSERT_MSG(condition, message) VKRT_UNUSED(condition)
#endif
#endif

#define VKRT_ASSERT(condition) VKRT_ASSERT_MSG(condition, "")

template <typename ResultValue>
inline auto VKRT_ASSERT_VK(ResultValue resultValue) -> decltype(resultValue.value) {
    VKRT_ASSERT_MSG(
        resultValue.result == vk::Result::eSuccess,
        "Vulkan error " << vk::to_string(resultValue.result));
    return resultValue.value;
}

inline void VKRT_ASSERT_VK(vk::Result result) {
    VKRT_ASSERT_MSG(result == vk::Result::eSuccess, "Vulkan error " << vk::to_string(result));
}
