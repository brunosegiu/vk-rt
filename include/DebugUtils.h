#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "Macros.h"
#include "VulkanBase.h"

#ifdef VKRT_PLATFORM_WINDOWS
#include <Windows.h>
#include <debugapi.h>
#endif

#ifdef VKRT_PLATFORM_WINDOWS
#define VKRT_DEBUG_BREAK() __debugbreak()                                                                                                                  \
#elif defined(VKRT_PLATFORM_LINUX)
#define VKRT_DEBUG_BREAK() __builtin_trap()
#endif

#ifdef VKRT_PLATFORM_WINDOWS
#define VKRT_LOG(message)                      \
    {                                          \
        std::wostringstream os_;               \
        os_ << message << std::endl;           \
        OutputDebugStringW(os_.str().c_str()); \
    }
#elif defined(VKRT_PLATFORM_LINUX)
#define VKRT_LOG(message)                      \
    {                                          \
        std::cout << message << std::endl;     \
    }
#endif

#ifdef VKRT_DEBUG
#define VKRT_ASSERT_MSG(condition, message)                                                                                                  \
    {                                                                                                                                        \
        if (!(condition)) {                                                                                                                  \
            std::string fileInfo = message;                                                                                                  \
            std::wstring wInfo = std::wstring(fileInfo.begin(), fileInfo.end());                                                             \
            VKRT_LOG(                                                                                                                        \
                "In file: " << __FILE__ << ", line: " << __LINE__ << " of function: " << __FUNCTION__ << "Condition failed : " << #condition \
                            << std::endl                                                                                                     \
                            << message << std::endl);                                                                                        \
            VKRT_DEBUG_BREAK();                                                                                                                  \
        }                                                                                                                                    \
    }
#else
#define VKRT_ASSERT_MSG(condition, message) VKRT_UNUSED(condition)
#endif

#define VKRT_ASSERT(condition) VKRT_ASSERT_MSG(condition, "")

template <typename ResultValue>
inline auto VKRT_ASSERT_VK(ResultValue resultValue) -> decltype(resultValue.value) {
    VKRT_ASSERT(resultValue.result == vk::Result::eSuccess);
    return resultValue.value;
}

inline void VKRT_ASSERT_VK(vk::Result result) {
    VKRT_ASSERT(result == vk::Result::eSuccess);
}
