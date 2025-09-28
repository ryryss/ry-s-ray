# ry‘s ray
ry is learning ray trace.

This project is dedicated to testing and validating various ray tracing algorithms and techniques, using Blender Cycles as the reference renderer.

## Env
This project requires the following third-party libraries:
-  tinygltf
-  glm
-  glfw

## Features
- receive a simple glb/gltf input
- use the first camera  and the first light found to render target
- [pbr : being implemented](https://github.com/mmp/pbr-book-website)

## Hardware
 intel i7-8700

## Perf
- v1.0.0 3000+ triangles take about 3 seconds to render on intel i7-8700
- v1.1.1 4000+ triangles simple PBR (SPP=64  bounces=3) with BVH 26s, without BVH 168s

## Version
- v1.0.0 use the first camera  and the first light found to render target
- v1.0.1 multi-primitive, light,  simple texture support
- v1.0.2 simple shadow support
- v1.0.3 fix some bug
- v1.1.0 PBRT base
- v1.2.0 refactor and reflection

## Test File
See https://github.com/KhronosGroup/glTF-Sample-Models or [branch model](https://github.com/ryryss/ry-s-ray/tree/model)

v1.1.1

Need to optimize sampling direction

![](rendering%20effect/v1.1.1.png)

v1.1.0

PBRT base,  sample implement Lambert diffuse reflection and cos hemispherical sampling

![](rendering%20effect/v1.1.0.png)

