#version 460
#extension GL_EXT_ray_tracing : require

const uint ShadowIndex = 1;

layout(location = ShadowIndex) rayPayloadInEXT bool isShadowed;

void main() {
  isShadowed = false;
}