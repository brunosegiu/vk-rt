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
    static Model* Load(Context* context, const std::string& path);

    Model(Context* context, const std::vector<Mesh*>& meshes);

    const std::vector<Mesh*>& GetMeshes() const { return mMeshes; }
    std::vector<Mesh::Description> GetDescriptions() const;

    ~Model();

private:
    Context* mContext;
    std::vector<Mesh*> mMeshes;
};

}  // namespace VKRT