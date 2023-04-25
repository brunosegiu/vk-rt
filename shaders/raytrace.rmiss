#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"

layout(binding = 4, set = 0, scalar) uniform LightMetadata {
    uint lightCount;
    vec3 sunDirection;
}
lightMetadata;

layout(location = ColorPayloadIndex) rayPayloadInEXT RayPayload rayPayload;

// From: https://github.com/nvpro-samples/nvpro_core/blob/master/nvvkhl/shaders/dh_sky.h
struct ProceduralSkyShaderParameters {
    vec3 directionToLight;
    float angularSizeOfLight;
    vec3 lightColor;
    float glowSize;
    vec3 skyColor;
    float glowIntensity;
    vec3 horizonColor;
    float horizonSize;
    vec3 groundColor;
    float glowSharpness;
    vec3 directionUp;
    float pad1;
};

ProceduralSkyShaderParameters initSkyShaderParameters(vec3 directionToLight) {
    ProceduralSkyShaderParameters parameters;
    parameters.directionToLight = directionToLight;
    parameters.angularSizeOfLight = 0.059F;
    parameters.lightColor = vec3(1.0F, 1.0F, 1.0F);
    parameters.skyColor = vec3(0.17F, 0.37F, 0.65F);
    parameters.horizonColor = vec3(0.50F, 0.70F, 0.92F);
    parameters.groundColor = vec3(0.62F, 0.59F, 0.55F);
    parameters.directionUp = vec3(0.F, 1.F, 0.F);
    parameters.horizonSize = 0.5F;
    parameters.glowSize = 0.091F;
    parameters.glowIntensity = 0.9F;
    parameters.glowSharpness = 4.F;

    return parameters;
}

vec3 getProceduralSkyColor(
    ProceduralSkyShaderParameters params,
    vec3 direction,
    float angularSizeOfPixel) {
    float elevation = asin(clamp(dot(direction, params.directionUp), -1.0F, 1.0F));
    float top = smoothstep(0.F, params.horizonSize, elevation);
    float bottom = smoothstep(0.F, params.horizonSize, -elevation);
    vec3 environment =
        mix(mix(params.horizonColor, params.groundColor, bottom), params.skyColor, top);

    float angleToLight = acos(clamp(dot(direction, params.directionToLight), 0.0F, 1.0F));
    float halfAngularSize = params.angularSizeOfLight * 0.5F;
    float lightIntensity = clamp(
        1.0F - smoothstep(
                   halfAngularSize - angularSizeOfPixel * 2.0F,
                   halfAngularSize + angularSizeOfPixel * 2.0F,
                   angleToLight),
        0.0F,
        1.0F);
    lightIntensity = pow(lightIntensity, 4.0F);
    float glow_input = clamp(
        2.0F * (1.0F - smoothstep(
                           halfAngularSize - params.glowSize,
                           halfAngularSize + params.glowSize,
                           angleToLight)),
        0.0F,
        1.0F);
    float glow_intensity = params.glowIntensity * pow(glow_input, params.glowSharpness);
    vec3 light = max(lightIntensity, glow_intensity) * params.lightColor;

    return environment + light;
}

void main() {
    ProceduralSkyShaderParameters params = initSkyShaderParameters(-lightMetadata.sunDirection.xyz);
    rayPayload.color += getProceduralSkyColor(params, gl_WorldRayDirectionEXT, 0);
}