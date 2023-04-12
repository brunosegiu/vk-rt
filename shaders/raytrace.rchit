#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

struct Description {
  uint64_t vertexBufferAddress;
  uint64_t indexBufferAddress;
};

struct Vertex {
  vec3 position;
  vec3 normal;
  vec2 texCoord;
};

struct Light {
  vec3 position;
  float intensity;
};

struct Material {
  vec3 albedo;
  float roughness;
  int albedoTextureIndex;
  int roughnessTextureIndex;
};

const int ColorIndex = 0;
const int ShadowIndex = 1;

layout(location = ColorIndex) rayPayloadInEXT vec3 color;
layout(location = ShadowIndex) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec2 hitAttributes;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {uvec3 i[]; };
layout(binding = 3, set = 0, scalar) buffer Description_ { Description i[]; } descriptions;
layout(binding = 4, set = 0) uniform LightMetadata {
  uint lightCount;
} lightMetadata;
layout(binding = 5, set = 0, scalar) buffer LightData {	Light lights[]; } lightData;
layout(binding = 6, set = 0) uniform sampler textureSampler;
layout(binding = 7, set = 0, scalar) buffer Material_ { Material values[]; } materials;
layout(binding = 8, set = 0) uniform texture2D sceneTextures[];

void main() {
  Description description = descriptions.i[gl_InstanceCustomIndexEXT];
  Indices indices = Indices(description.indexBufferAddress);
  Vertices vertices = Vertices(description.vertexBufferAddress);

  uvec3 triangleIndices = indices.i[gl_PrimitiveID];
  Vertex v0 = vertices.v[triangleIndices.x];
  Vertex v1 = vertices.v[triangleIndices.y];
  Vertex v2 = vertices.v[triangleIndices.z];

  const vec3 barycentricCoords = vec3(1.0f - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);
  
  const vec3 position = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
  const vec3 worldSpacePosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));
  
  const vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
  const vec3 worldSpaceNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

  const vec2 texCoord =  v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z;

  Material material = materials.values[gl_InstanceCustomIndexEXT];

  color = vec3(0.0);
  vec3 albedo = material.albedo.rgb;
  if (material.albedoTextureIndex >= 0) {
    albedo = texture(sampler2D(sceneTextures[material.albedoTextureIndex], textureSampler), texCoord).rgb;
  }
  for (uint lightIndex = 0; lightIndex < lightMetadata.lightCount; ++lightIndex) {
    Light light = lightData.lights[lightIndex];
    const vec3 lightPos = light.position;
    vec3 lightDir = lightPos - worldSpacePosition;
    const float lightDistance = length(lightDir);
    const float lightIntensity = light.intensity / (lightDistance * lightDistance);
    lightDir = lightDir / lightDistance;
    const float nDotL = max(dot(worldSpaceNormal, lightDir), 0.0);
    float attenuation = 1.0;
    if(nDotL > 0.0) {
      const float tMin   = 0.001;
      const float tMax   = lightDistance;
      const vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
      const vec3 rayDir = lightDir;
      const uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
      isShadowed = true;
      traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1, origin, tMin, rayDir, tMax, ShadowIndex);

      if(isShadowed) {
        attenuation = 0.0;
      }
    }
    color += color + albedo * nDotL * lightIntensity * attenuation;
  }
}
