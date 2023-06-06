#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"

layout(location = ShadowPayloadIndex) rayPayloadInEXT float shadowAttenuation;

void main() {
    shadowAttenuation = 1.0f;
}