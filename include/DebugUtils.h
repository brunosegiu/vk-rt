#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "Macros.h"
#include "VulkanBase.h"

#include <Windows.h>
#include <debugapi.h>

#define VKRT_LOG(message)                      \
    {                                          \
        std::wostringstream os_;               \
        os_ << message << std::endl;           \
        OutputDebugStringW(os_.str().c_str()); \
    }

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
            __debugbreak();                                                                                                                  \
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
