#include "ResourceLoader.h"

#include <algorithm>
#include <string>

#include <Windows.h>

#include "ShaderResources.h"

namespace VKRT {

Resource ResourceLoader::Load(const Resource::Id& resourceId) {
    uint32_t actualId = 0;
    switch (resourceId) {
        case Resource::Id::GenerateShader:
            actualId = VKRT_RESOURCE_COMPUTE_SHADER;
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

void ResourceLoader::CleanUp(Resource& resource) {
    resource.buffer = nullptr;
    resource.size = 0;
}

}  // namespace VKRT