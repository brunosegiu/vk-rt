#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"

layout(location = ColorPayloadIndex) rayPayloadInEXT RayPayload rayPayload;
layout(location = ShadowPayloadIndex) rayPayloadEXT float shadowAttenuation;
hitAttributeEXT vec2 hitAttributes;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(buffer_reference, scalar) buffer Vertices {
    Vertex values[];
};
layout(buffer_reference, scalar) buffer Indices {
    uvec3 values[];
};
layout(binding = 3, set = 0, scalar) buffer Description_ {
    MeshDescription values[];
}
descriptions;
layout(binding = 4, set = 0) uniform LightMetadata {
    uint lightCount;
}
lightMetadata;
layout(binding = 5, set = 0, scalar) buffer LightData {
    Light values[];
}
lights;
layout(binding = 6, set = 0) uniform sampler textureSampler;
layout(binding = 7, set = 0, scalar) buffer Material_ {
    Material values[];
}
materials;
layout(binding = 8, set = 0) uniform texture2D sceneTextures[];

Vertex unpackInstanceVertex(const int intanceId) {
    MeshDescription description = descriptions.values[intanceId];
    Indices indices = Indices(description.indexBufferAddress);
    Vertices vertices = Vertices(description.vertexBufferAddress);

    uvec3 triangleIndices = indices.values[gl_PrimitiveID];
    Vertex v0 = vertices.values[triangleIndices.x];
    Vertex v1 = vertices.values[triangleIndices.y];
    Vertex v2 = vertices.values[triangleIndices.z];

    const vec3 barycentricCoords =
        vec3(1.0f - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);

    const vec3 position = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y +
                          v2.position * barycentricCoords.z;
    const vec3 worldSpacePosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

    const vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y +
                        v2.normal * barycentricCoords.z;
    const vec3 worldSpaceNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

    const vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y +
                          v2.texCoord * barycentricCoords.z;

    return Vertex(worldSpacePosition, worldSpaceNormal, texCoord);
}

Material unpackInstanceMaterial(const int intanceId) {
    return materials.values[intanceId];
}

vec3 getAlbedo(const Material material, const vec2 texCoord) {
    vec3 albedo = material.albedo.rgb;
    if (material.albedoTextureIndex >= 0) {
        albedo =
            texture(sampler2D(sceneTextures[material.albedoTextureIndex], textureSampler), texCoord)
                .rgb;
    }
    return albedo;
}

float getRoughness(const Material material, const vec2 texCoord) {
    float roughness = material.roughness;
    if (material.roughnessTextureIndex >= 0) {
        roughness = texture(
                        sampler2D(sceneTextures[material.roughnessTextureIndex], textureSampler),
                        texCoord)
                        .r;
    }
    return roughness;
}

vec3 getDirectIllumination(const Vertex vertex, const vec3 albedo) {
    vec3 color = albedo * AmbientTerm;
    for (uint lightIndex = 0; lightIndex < lightMetadata.lightCount; ++lightIndex) {
        Light light = lights.values[lightIndex];
        vec3 lightDir;
        float lightIntensity;
        switch (light.type) {
            case LightTypeDirectional: {
                lightDir = -light.directionOrPosition;
                lightIntensity = light.intensity;
            } break;
            case LightTypePoint: {
                const vec3 lightPos = light.directionOrPosition;
                lightDir = lightPos - vertex.position;
                const float lightDistance = length(lightDir);
                lightIntensity = light.intensity / (lightDistance * lightDistance);
                lightDir = lightDir / lightDistance;
            } break;
        }
        const float nDotL = max(dot(vertex.normal, lightDir), 0.0);
        if (nDotL > 0.0) {
            const vec3 shadowRayOrigin = vertex.position;
            const vec3 shadowRayDirection = lightDir;
            shadowAttenuation = 0.0f;
            traceRayEXT(
                topLevelAS,
                gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT |
                    gl_RayFlagsSkipClosestHitShaderEXT,
                DefaultCullMask,
                DefaultSBTOffset,
                DefaultSBTStride,
                ShadowMissIndex,
                shadowRayOrigin,
                TMin,
                shadowRayDirection,
                TMax,
                ShadowPayloadIndex);
            color += color + albedo * (nDotL * lightIntensity * shadowAttenuation);
        }
    }
    return color;
}

void main() {
    Vertex vertex = unpackInstanceVertex(gl_InstanceCustomIndexEXT);
    Material material = unpackInstanceMaterial(gl_InstanceCustomIndexEXT);

    vec3 albedo = getAlbedo(material, vertex.texCoord);
    float roughness = getRoughness(material, vertex.texCoord);

    vec3 directIlluminationTerm = vec3(0.0);
    if (material.indexOfRefraction <= 0.0) {
        directIlluminationTerm = getDirectIllumination(vertex, albedo);
    }

    const vec3 rayOrigin = vertex.position;
    const vec3 rayDirection = normalize(gl_WorldRayDirectionEXT);

    rayPayload.color += directIlluminationTerm * rayPayload.weight;
    rayPayload.depth += 1;
    rayPayload.weight *= material.metallic;

    if (material.metallic > MetallicCuttoff && rayPayload.depth < 10) {
        const vec3 reflectionOrigin = rayOrigin;
        const vec3 reflectionDirection = reflect(rayDirection, vertex.normal);
        traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            DefaultCullMask,
            DefaultSBTOffset,
            DefaultSBTStride,
            ColorMissIndex,
            reflectionOrigin,
            TMin,
            reflectionDirection,
            TMax,
            ColorPayloadIndex);
    }

    if (material.indexOfRefraction > 0.0 && rayPayload.depth < 10) {
        rayPayload.depth += 1;
        rayPayload.weight = vec3(1.0);
        const bool isFrontFacing = gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT;
        float refractionRatio;
        vec3 refractionNormal;
        if (isFrontFacing) {
            refractionRatio = 1.0f / material.indexOfRefraction;
            refractionNormal = vertex.normal;
        } else {
            refractionRatio = material.indexOfRefraction;
            refractionNormal = -vertex.normal;
        }
        vec3 refractionOrigin = rayOrigin - refractionNormal * Bias;
        vec3 refractedDirection = refract(rayDirection, refractionNormal, refractionRatio);
        traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            DefaultCullMask,
            DefaultSBTOffset,
            DefaultSBTStride,
            ColorMissIndex,
            refractionOrigin,
            TMin,
            refractedDirection,
            TMax,
            ColorPayloadIndex);
    }
}
