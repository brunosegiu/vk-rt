# Real-time Vulkan raytracer

For a long time I've wondered what a real-time Whitted-ish raytracer would look like, so I took the challenge of learning the raytracing Vulkan extensions. Indirect diffuse illumination is still coming but planned. This is mostly a playground and a learning exercise, trying to avoid path-tracing because I don't want it to rely on denoising.

## Features
### - GLTF loading
### - PBR direct illumination

![2023-04-24 23-59-22](https://user-images.githubusercontent.com/38514393/234164339-a2af994e-a86c-4121-a4bc-a132582e2b21.gif)

### - Fresnel transmission
### - Perfect reflections
### - Per-frame TLAS updates

![2023-04-25 00-00-47](https://user-images.githubusercontent.com/38514393/234164753-7b109cc3-c100-4279-b8d3-256849248585.gif)

### - Directional, point lights
### - Scene texture pool with bindless descriptors
![2023-04-25 00-01-48(2)](https://user-images.githubusercontent.com/38514393/234165396-b171580d-1844-46bb-84a3-1634937bd21c.gif)

## References:
- [Nvidia raytacing tutorial](https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/)
- [SaschaWillems Vulkan examples](https://github.com/SaschaWillems/Vulkan)
- [Scratchapixel](https://www.scratchapixel.com/)
- [LearnOpenGL PBR](https://learnopengl.com/PBR/Lighting)
