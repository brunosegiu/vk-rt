#include "ResourceLoader.h"

#include <algorithm>
#include <string>

#ifdef VKRT_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef VKRT_PLATFORM_LINUX
#endif

#include "ShaderResources.h"

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
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&ResourceLoader::Load, &module);
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
#elif defined(VKRT_PLATFORM_LINUX)
Resource ResourceLoader::Load(const Resource::Id& resourceId) {
        return {nullptr, 0};
}
#endif

void ResourceLoader::CleanUp(Resource& resource) {
    delete[] resource.buffer;
    resource.buffer = nullptr;
    resource.size = 0;
}

}  // namespace VKRT
