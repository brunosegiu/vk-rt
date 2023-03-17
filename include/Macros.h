#pragma once

#define VKRT_UNUSED(arg) (void)(arg)

// Debug macro
#ifdef VKRT_PLATFORM_WINDOWS

#ifdef _DEBUG
#define VKRT_DEBUG
#endif

#else

#ifdef DEBUG
#define VKRT_DEBUG
#endif

#endif