#pragma once

#include "glm/glm.hpp"

#include "Context.h"
#include "Material.h"
#include "Mesh.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Model : public RefCountPtr {
public:
    static Model* Load(ScopedRefPtr<Context>, const std::string& path);

    Model(ScopedRefPtr<Context>, const std::vector<ScopedRefPtr<Mesh>>& meshes);

    const std::vector<ScopedRefPtr<Mesh>>& GetMeshes() const { return mMeshes; }
    std::vector<Mesh::Description> GetDescriptions() const;

    ~Model();

private:
    ScopedRefPtr<Context> mContext;
    std::vector<ScopedRefPtr<Mesh>> mMeshes;
};

}  // namespace VKRT