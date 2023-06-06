#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"
#include "proceduralSky.glsl"

layout(binding = 4, set = 0, scalar) uniform LightMetadata {
    uint lightCount;
    vec3 sunDirection;
}
lightMetadata;

layout(location = ColorPayloadIndex) rayPayloadInEXT RayPayload rayPayload;

void main() {
    ProceduralSkyShaderParameters params = initSkyShaderParameters(-lightMetadata.sunDirection.xyz);
    rayPayload.color += getProceduralSkyColor(params, gl_WorldRayDirectionEXT, 0);
}