#include "ResourceLoader.h"

#include <algorithm>
#include <string>

#ifdef VKRT_PLATFORM_WINDOWS
#include <Windows.h>
#include "ShaderResources.h"
#endif

#ifdef VKRT_PLATFORM_LINUX
extern "C" {
#include "incbin.h"
}

namespace VKRT {
INCBIN(GenShader, "raytrace.rgen.spv");
INCBIN(HitShader, "raytrace.rchit.spv");
INCBIN(MissShader, "raytrace.rmiss.spv");
}  // namespace VKRT
#endif

namespace VKRT {

#ifdef VKRT_PLATFORM_WINDOWS
Resource ResourceLoader::Load(const Resource::Id& resourceId) {
    uint32_t actualId = 0;
    switch (resourceId) {
        case Resource::Id::GenShader:
            actualId = VKRT_RESOURCE_RAYTRACE_GEN_SHADER;
            break;
        case Resource::Id::HitShader:
            actualId = VKRT_RESOURCE_RAYTRACE_HIT_SHADER;
            break;
        case Resource::Id::MissShader:
            actualId = VKRT_RESOURCE_RAYTRACE_MISS_SHADER;
            break;
        default:
            return {nullptr, 0};
    }

    HMODULE module = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&ResourceLoader::Load,
        &module);
    HRSRC resource = FindResource(module, MAKEINTRESOURCE(actualId), RT_RCDATA);
    if (resource != nullptr) {
        size_t bufferSize = SizeofResource(module, resource);
        HGLOBAL data = LoadResource(module, resource);
        if (data != nullptr && bufferSize != 0) {
            auto resourceBuffer = reinterpret_cast<uint8_t*>(LockResource(data));
            auto buffer = new uint8_t[bufferSize];
            std::copy_n(resourceBuffer, bufferSize, buffer);
            return {buffer, bufferSize};
        }
    }
    return {nullptr, 0};
}
#endif

#ifdef VKRT_PLATFORM_LINUX
Resource ResourceLoader::Load(const Resource::Id& resourceId) {
    switch (resourceId) {
        case Resource::Id::GenShader: {
            return Resource{.buffer = gGenShaderData, .size = gGenShaderSize};
        } break;
        case Resource::Id::HitShader: {
            return Resource{.buffer = gHitShaderData, .size = gHitShaderSize};
        } break;
        case Resource::Id::MissShader: {
            return Resource{.buffer = gMissShaderData, .size = gMissShaderSize};
        } break;
        default:
            return {nullptr, 0};
    }
}
#endif

void ResourceLoader::CleanUp(Resource& resource) {
#ifndef VKRT_PLATFORM_LINUX
    delete[] resource.buffer;
#endif
    resource.buffer = nullptr;
    resource.size = 0;
}

}  // namespace VKRT
