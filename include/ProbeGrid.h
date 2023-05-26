#pragma once

#include "glm/glm.hpp"

#include "RefCountPtr.h"
#include "Texture.h"

namespace VKRT {

class ProbeGrid : public RefCountPtr {
public:
    ProbeGrid();

    virtual ~ProbeGrid();

private:
    ScopedRefPtr<Texture> mProbesTexture;
    glm::vec3 mOrigin;
    glm::vec3 mSize;
    glm::uvec3 mResolution;
};

}  // namespace VKRT