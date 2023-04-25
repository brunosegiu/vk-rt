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
layout(binding = 4, set = 0, scalar) uniform LightMetadata {
    uint lightCount;
    vec3 sunDirection;
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

void getRoughnessAndMetallic(
    const Material material,
    const vec2 texCoord,
    out float roughness,
    out float metallic) {
    roughness = material.roughness;
    metallic = material.metallic;
    if (material.roughnessTextureIndex >= 0) {
        vec4 textureSample = texture(
            sampler2D(sceneTextures[material.roughnessTextureIndex], textureSampler),
            texCoord);
        metallic = textureSample.b;
        roughness = textureSample.g;
    }
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float fresnelSchlick(float cosTheta, float r0) {
    return r0 + (1.0 - r0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = Pi * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float traceShadowRay(const vec3 origin, const vec3 direction, float distance) {
    shadowAttenuation = 0.0f;
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT |
            gl_RayFlagsSkipClosestHitShaderEXT,
        DefaultCullMask,
        DefaultSBTOffset,
        DefaultSBTStride,
        ShadowMissIndex,
        origin,
        TMin,
        direction,
        distance,
        ShadowPayloadIndex);
    return shadowAttenuation;
}

float fresnel(const vec3 I, const vec3 N, const float ior) {
    float cosi = dot(I, N);
    float etai = 1, etat = ior;
    if (cosi > 0) {
        float a = etai;
        etai = etat;
        etat = a;
    }
    // Compute sini using Snell's law
    float sint = etai / etat * sqrt(max(0.f, 1 - cosi * cosi));
    if (sint >= 1) {
        return 1;
    } else {
        float cost = sqrt(max(0.f, 1 - sint * sint));
        cosi = abs(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        return (Rs * Rs + Rp * Rp) / 2;
    }
}

void main() {
    if (rayPayload.depth > MaxRecursionLevel) {
        rayPayload.color = vec3(0.0f);
        return;
    }

    rayPayload.depth += 1;

    const Vertex vertex = unpackInstanceVertex(gl_InstanceCustomIndexEXT);
    const Material material = unpackInstanceMaterial(gl_InstanceCustomIndexEXT);

    const vec3 albedo = getAlbedo(material, vertex.texCoord);
    float roughness, metallic;
    getRoughnessAndMetallic(material, vertex.texCoord, roughness, metallic);

    const float indexOfRefraction = material.indexOfRefraction;

    const vec3 rayOrigin = vertex.position;
    const vec3 D = normalize(gl_WorldRayDirectionEXT);
    const vec3 N = vertex.normal;
    const vec3 V = -D;

    const vec3 f0 = mix(vec3(0.04), albedo, metallic);
    const vec3 diffuseColor = albedo * vec3(1.0f - f0) * (1.0f - metallic);
    vec3 color = vec3(0.0f);
    if (indexOfRefraction < 0.0f) {
        color += diffuseColor * AmbientTerm;
        for (uint lightIndex = 0; lightIndex < lightMetadata.lightCount; ++lightIndex) {
            Light light = lights.values[lightIndex];
            vec3 L;
            float lightIntensity = light.intensity;
            float lightDistance = TMax;
            switch (light.type) {
                case LightTypeDirectional: {
                    L = -light.directionOrPosition;
                } break;
                case LightTypePoint: {
                    L = light.directionOrPosition - vertex.position;
                    lightDistance = length(L);
                    lightIntensity = lightIntensity / (lightDistance * lightDistance);
                    L = L / lightDistance;
                } break;
            }
            const vec3 H = normalize(V + L);
            const float nDotL = max(dot(vertex.normal, L), 0.0);
            if (nDotL > 0.0) {
                const float shadowAttenuation = traceShadowRay(vertex.position, L, lightDistance);
                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                vec3 F = fresnelSchlick(max(dot(H, V), 0.0), f0);
                vec3 kS = F;
                vec3 kD = vec3(1.0) - kS;
                kD *= 1.0 - metallic;
                vec3 numerator = NDF * G * F;
                float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
                vec3 specular = numerator / denominator;
                color += shadowAttenuation * nDotL * lightIntensity *
                         (kD * diffuseColor / Pi + specular);
            }
        }
    }

    if (metallic > 0.0f) {
        const vec3 reflectionOrigin = vertex.position;
        const vec3 reflectionDirection = reflect(D, N);
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
        color += metallic * rayPayload.color;
    } else if (indexOfRefraction > 0.0) {
        const float nDotD = dot(N, D);
        vec3 refrNormal;
        float refrEta;
        if (nDotD > 0.0f) {
            refrNormal = -N;
            refrEta = indexOfRefraction;
        } else {
            refrNormal = N;
            refrEta = 1.0f / indexOfRefraction;
        }
        const vec3 refractionOrigin = vertex.position;
        float fresnelTerm = fresnel(D, N, indexOfRefraction);
        vec3 refractionColor = vec3(0.0f);
        vec3 reflectionColor = vec3(0.0f);

        if (nDotD < 0.0f && fresnelTerm > 0.0f) {
            const vec3 reflectionDirection = reflect(D, N);
            traceRayEXT(
                topLevelAS,
                gl_RayFlagsOpaqueEXT,
                DefaultCullMask,
                DefaultSBTOffset,
                DefaultSBTStride,
                ColorMissIndex,
                refractionOrigin,
                TMin,
                reflectionDirection,
                TMax,
                ColorPayloadIndex);
            reflectionColor = rayPayload.color;
        }

        if (fresnelTerm < 1.0f) {
            const vec3 refractionDirection = refract(D, refrNormal, refrEta);
            traceRayEXT(
                topLevelAS,
                gl_RayFlagsOpaqueEXT,
                DefaultCullMask,
                DefaultSBTOffset,
                DefaultSBTStride,
                ColorMissIndex,
                refractionOrigin,
                TMin,
                refractionDirection,
                TMax,
                ColorPayloadIndex);
            refractionColor = rayPayload.color;
        }
        color += mix(refractionColor, reflectionColor, fresnelTerm);
    }

    rayPayload.color = color;
}
