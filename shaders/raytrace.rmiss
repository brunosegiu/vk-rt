#version 460
#extension GL_EXT_ray_tracing : enable

const int ColorIndex = 0;

struct RayPayload {
    vec3 color;
    vec3 weight;
	int depth;
};

layout(location = ColorIndex) rayPayloadInEXT RayPayload rayPayload;

// From: https://github.com/nvpro-samples/nvpro_core/blob/master/nvvkhl/shaders/dh_sky.h
struct ProceduralSkyShaderParameters
{
  vec3  directionToLight;
  float angularSizeOfLight;

  vec3  lightColor;
  float glowSize;

  vec3  skyColor;
  float glowIntensity;

  vec3  horizonColor;
  float horizonSize;

  vec3  groundColor;
  float glowSharpness;

  vec3  directionUp;
  float pad1;
};

ProceduralSkyShaderParameters initSkyShaderParameters()
{
  ProceduralSkyShaderParameters p;
  p.directionToLight   = vec3(0.0F, 0.707F, 0.707F);
  p.angularSizeOfLight = 0.059F;
  p.lightColor         = vec3(1.0F, 1.0F, 1.0F);
  p.skyColor           = vec3(0.17F, 0.37F, 0.65F);
  p.horizonColor       = vec3(0.50F, 0.70F, 0.92F);
  p.groundColor        = vec3(0.62F, 0.59F, 0.55F);
  p.directionUp        = vec3(0.F, 1.F, 0.F);
  p.horizonSize        = 0.5F;    // +/- degrees
  p.glowSize           = 0.091F;  // degrees, starting from the edge of the light disk
  p.glowIntensity      = 0.9F;    // [0-1] relative to light intensity
  p.glowSharpness      = 4.F;     // [1-10] is the glow power exponent

  return p;
}

vec3 proceduralSky(ProceduralSkyShaderParameters params, vec3 direction, float angularSizeOfPixel)
{
  float elevation   = asin(clamp(dot(direction, params.directionUp), -1.0F, 1.0F));
  float top         = smoothstep(0.F, params.horizonSize, elevation);
  float bottom      = smoothstep(0.F, params.horizonSize, -elevation);
  vec3  environment = mix(mix(params.horizonColor, params.groundColor, bottom), params.skyColor, top);

  float angle_to_light    = acos(clamp(dot(direction, params.directionToLight), 0.0F, 1.0F));
  float half_angular_size = params.angularSizeOfLight * 0.5F;
  float light_intensity =
      clamp(1.0F - smoothstep(half_angular_size - angularSizeOfPixel * 2.0F, half_angular_size + angularSizeOfPixel * 2.0F, angle_to_light),
            0.0F, 1.0F);
  light_intensity = pow(light_intensity, 4.0F);
  float glow_input =
      clamp(2.0F * (1.0F - smoothstep(half_angular_size - params.glowSize, half_angular_size + params.glowSize, angle_to_light)),
            0.0F, 1.0F);
  float glow_intensity = params.glowIntensity * pow(glow_input, params.glowSharpness);
  vec3  light          = max(light_intensity, glow_intensity) * params.lightColor;

  return environment + light;
}


void main() {
    rayPayload.color += proceduralSky(initSkyShaderParameters(), gl_WorldRayDirectionEXT, 0);
    rayPayload.depth = -1;
}