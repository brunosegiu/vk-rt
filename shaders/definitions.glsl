#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

const int ColorPayloadIndex = 0;
const int ShadowPayloadIndex = 1;

const int ColorMissIndex = 0;
const int ShadowMissIndex = 1;

const float TMin = 0.001f;
const float TMax = 1000.0f;
const uint DefaultCullMask = 0xFF;
const uint DefaultSBTOffset = 0;
const uint DefaultSBTStride = 0;

const vec3 AmbientTerm = vec3(0.01);

const float Bias = 0.01f;

const float Pi = 3.14159265359f;

const uint MaxRecursionLevel = 8;

struct MeshDescription {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
};

const uint LightTypeDirectional = 0;
const uint LightTypePoint = 1;

struct Light {
    uint type;
    vec3 directionOrPosition;
    float intensity;
};

struct Material {
    vec3 albedo;
    float roughness;
    float metallic;
    float indexOfRefraction;
    int albedoTextureIndex;
    int roughnessTextureIndex;
};

struct RayPayload {
    vec3 color;
    vec3 position;
    vec3 normal;
    float metallic;
    float indexOfRefraction;
    float metallicFalloff;
};