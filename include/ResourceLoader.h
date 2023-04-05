#pragma once

#include <cstddef>
#include <cstdint>

#include "Macros.h"

namespace VKRT {

struct Resource {
    const uint8_t* buffer;
    size_t size;

    enum class Id { GenShader, HitShader, MissShader, ShadowMissShader };
};

class ResourceLoader {
public:
    static Resource Load(const Resource::Id& resourceId);
    static void CleanUp(Resource& resource);
};

}  // namespace VKRT
