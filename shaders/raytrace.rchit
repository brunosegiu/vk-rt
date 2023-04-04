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

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {uvec3 i[]; };
layout(binding = 3, set = 0, scalar) buffer Description_ { Description i[]; } descriptions;

void main() {
  Description description = descriptions.i[gl_InstanceCustomIndexEXT];
  Indices indices = Indices(description.indexBufferAddress);
  Vertices vertices = Vertices(description.vertexBufferAddress);

  uvec3 triangleIndices = indices.i[gl_PrimitiveID];
  Vertex v0 = vertices.v[triangleIndices.x];
  Vertex v1 = vertices.v[triangleIndices.y];
  Vertex v2 = vertices.v[triangleIndices.z];

  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  
  const vec3 position = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y + v2.position * barycentricCoords.z;
  const vec3 worldSpacePosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));
  
  const vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
  const vec3 worldSpaceNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

  vec3 lightPos = vec3(0.0f, -5.0f, -5.0f);
  vec3 lightDir = lightPos - worldSpacePosition;
  const float lightDistance = length(lightDir);
  const float lightIntensity = 1000.0f / (lightDistance * lightDistance);
  lightDir = lightDir / lightDistance;

  float nDotL = max(dot(worldSpaceNormal, lightDir), 0.0);
  vec3 color = vec3(0.7f, 0.7f, 0.7f);
  hitValue = color * nDotL + vec3(0.07f);
}
