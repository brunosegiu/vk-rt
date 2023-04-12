#version 460
#extension GL_EXT_ray_tracing : enable

const int ColorIndex = 0;

struct RayPayload {
    vec3 color;
    float distance;
    vec3 normal;
    float reflectiveness;
};

layout(location = ColorIndex) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.color = vec3(0.0);
    rayPayload.distance = -1.0;
    rayPayload.normal = vec3(0.0);
    rayPayload.reflectiveness = 0.0;
}