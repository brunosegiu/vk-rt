#include "Model.h"

#include "nlohmann/json.hpp"
#include "tiny_gltf.h"

#include "DebugUtils.h"
#include "Material.h"
#include "Texture.h"

namespace VKRT {

Model* Model::Load(Context* context, const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool isProperlyLoaded = false;
    if (path.ends_with(".gltf")) {
        isProperlyLoaded = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    } else {
        isProperlyLoaded = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    }
    constexpr int32_t invalidIndex = -1;
    std::vector<Mesh*> meshes;
    if (isProperlyLoaded) {
        if (!model.nodes.empty()) {
            const int32_t meshIndex = model.nodes.front().mesh;
            const tinygltf::Mesh& mesh = model.meshes[meshIndex];
            for (const tinygltf::Primitive& primitive : mesh.primitives) {
                const std::string positionName = "POSITION";
                const std::string normalName = "NORMAL";
                const std::string texCoordName = "TEXCOORD_0";

                const std::map<std::string, int>& attributes = primitive.attributes;
                const bool hasAttributes = attributes.find(positionName) != attributes.end() &&
                                           attributes.find(normalName) != attributes.end() &&
                                           attributes.find(texCoordName) != attributes.end();

                if (!hasAttributes) {
                    return nullptr;
                }

                const tinygltf::Accessor& positionAccessor =
                    model.accessors[attributes.at(positionName)];
                std::vector<glm::vec3> positions(positionAccessor.count);
                {
                    const tinygltf::BufferView& positionBufferView =
                        model.bufferViews[positionAccessor.bufferView];
                    const tinygltf::Buffer& positionBuffer =
                        model.buffers[positionBufferView.buffer];
                    const size_t positionBufferOffset =
                        positionBufferView.byteOffset + positionAccessor.byteOffset;
                    size_t vetexStride = positionAccessor.ByteStride(positionBufferView);
                    const unsigned char* positionData = &positionBuffer.data[positionBufferOffset];

                    const uint32_t positionCount = static_cast<uint32_t>(positionAccessor.count);
                    for (uint32_t positionIndex = 0; positionIndex < positionCount;
                         ++positionIndex) {
                        const float* positionDataFloat = reinterpret_cast<const float*>(
                            &positionData[vetexStride * positionIndex]);
                        positions[positionIndex] = glm::vec3(
                            positionDataFloat[0],
                            -positionDataFloat[1],
                            positionDataFloat[2]);
                    }
                }

                const tinygltf::Accessor& normalAccessor =
                    model.accessors[attributes.at(normalName)];
                std::vector<glm::vec3> normals(normalAccessor.count);
                {
                    const tinygltf::BufferView& normalBufferView =
                        model.bufferViews[normalAccessor.bufferView];
                    const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
                    const size_t normalBufferOffset =
                        normalBufferView.byteOffset + normalAccessor.byteOffset;
                    size_t vetexStride = normalAccessor.ByteStride(normalBufferView);
                    const unsigned char* normalData = &normalBuffer.data[normalBufferOffset];

                    const uint32_t normalCount = static_cast<uint32_t>(normalAccessor.count);
                    for (uint32_t normalIndex = 0; normalIndex < normalCount; ++normalIndex) {
                        const float* normalDataFloat =
                            reinterpret_cast<const float*>(&normalData[vetexStride * normalIndex]);
                        normals[normalIndex] =
                            glm::vec3(normalDataFloat[0], normalDataFloat[1], normalDataFloat[2]);
                    }
                }

                const tinygltf::Accessor& texCoordAccessor =
                    model.accessors[attributes.at(texCoordName)];
                std::vector<glm::vec2> texCoords(texCoordAccessor.count);
                {
                    const tinygltf::BufferView& texCoordBufferView =
                        model.bufferViews[texCoordAccessor.bufferView];
                    const tinygltf::Buffer& texCoordBuffer =
                        model.buffers[texCoordBufferView.buffer];
                    const size_t texCoordBufferOffset =
                        texCoordBufferView.byteOffset + texCoordAccessor.byteOffset;
                    size_t vetexStride = texCoordAccessor.ByteStride(texCoordBufferView);
                    const unsigned char* texCoordData = &texCoordBuffer.data[texCoordBufferOffset];

                    const uint32_t texCoordCount = static_cast<uint32_t>(texCoordAccessor.count);
                    for (uint32_t texCoordIndex = 0; texCoordIndex < texCoordCount;
                         ++texCoordIndex) {
                        const float* texCoordDataFloat = reinterpret_cast<const float*>(
                            &texCoordData[vetexStride * texCoordIndex]);
                        texCoords[texCoordIndex] =
                            glm::vec2(texCoordDataFloat[0], texCoordDataFloat[1]);
                    }
                }

                std::vector<Mesh::Vertex> vertices;
                vertices.reserve(positions.size());
                for (size_t vertexIndex = 0; vertexIndex < positions.size(); ++vertexIndex) {
                    Mesh::Vertex vertex{
                        .position = positions[vertexIndex],
                        .normal = normals[vertexIndex],
                        .texCoord = texCoords[vertexIndex]};
                    vertices.push_back(vertex);
                }

                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                std::vector<glm::uvec3> indices;
                {
                    const uint32_t indexCount = static_cast<uint32_t>(indexAccessor.count);
                    const tinygltf::BufferView& indexBufferView =
                        model.bufferViews[indexAccessor.bufferView];
                    const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                    const size_t indexOffset =
                        indexBufferView.byteOffset + indexAccessor.byteOffset;
                    const uint16_t* pIndexData =
                        reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexOffset]);
                    indices.reserve(indexCount / 3);
                    for (uint32_t i = 0; i < indexCount; i += 3) {
                        const glm::uvec3 triangle =
                            glm::uvec3(pIndexData[i], pIndexData[i + 1], pIndexData[i + 2]);
                        indices.emplace_back(triangle);
                    }
                }

                const int32_t materialIndex = primitive.material;
                Material* material = nullptr;
                if (materialIndex >= 0) {
                    const tinygltf::Material& gltfMaterial = model.materials[materialIndex];

                    const std::vector<double>& baseColor =
                        gltfMaterial.pbrMetallicRoughness.baseColorFactor;
                    glm::vec3 albedo = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);
                    float roughness = gltfMaterial.pbrMetallicRoughness.roughnessFactor;

                    const int32_t albedoTextureIndex =
                        gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
                    Texture* albedoTexture = nullptr;
                    if (albedoTextureIndex >= 0) {
                        const tinygltf::Texture& texture = model.textures[albedoTextureIndex];
                        const tinygltf::Image& image = model.images[texture.source];
                        albedoTexture = new Texture(
                            context,
                            image.width,
                            image.height,
                            vk::Format::eR8G8B8A8Unorm,
                            image.image.data(),
                            image.image.size());
                    }

                    const int32_t roughnessTextureIndex =
                        gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                    gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                    Texture* roughnessTexture = nullptr;
                    if (roughnessTextureIndex >= 0) {
                        const tinygltf::Texture& texture = model.textures[roughnessTextureIndex];
                        const tinygltf::Image& image = model.images[texture.source];
                        roughnessTexture = new Texture(
                            context,
                            image.width,
                            image.height,
                            vk::Format::eR8G8B8A8Unorm,
                            image.image.data(),
                            image.image.size());
                    }

                    material = new Material(albedo, roughness, albedoTexture, roughnessTexture);

                    if (albedoTexture != nullptr) {
                        albedoTexture->Release();
                    }

                    if (roughnessTexture != nullptr) {
                        roughnessTexture->Release();
                    }
                } else {
                    material = new Material();
                }

                Mesh* mesh = new Mesh(context, vertices, indices, material);
                material->Release();
                meshes.push_back(mesh);
            }
        }

        return new Model(context, meshes);
    }
    return nullptr;
}

Model::Model(Context* context, const std::vector<Mesh*>& meshes)
    : mContext(context), mMeshes(meshes) {
    mContext->AddRef();
}

std::vector<Mesh::Description> Model::GetDescriptions() const {
    std::vector<Mesh::Description> descriptions;
    for (const Mesh* mesh : mMeshes) {
        descriptions.push_back(mesh->GetDescription());
    }
    return descriptions;
}

Model::~Model() {
    for (Mesh* mesh : mMeshes) {
        mesh->Release();
    }
    mContext->Release();
}

}  // namespace VKRT