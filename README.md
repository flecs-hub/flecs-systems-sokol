# flecs-systems-sokol
Sokol based rendering module for [Flecs](https://github.com/SanderMertens/flecs). This renderer should only be used as example code or for demonstration purposes and is not intended or ready for production.

## Features
- Runs on webasm, macOS, Linux, Windows
- Box & Rectangle primitives
- Automatic instancing
- Phong shading model
- Directional (sun) light
- Ambient light
- Emissive & shiny materials
- Depth prepass
- Shadow mapping
- Atmosphere rendering
- Post process effects:
  - Exponential fog
  - Ambient occlusion
  - Bloom
  - HDR & gamma correction

## Future work
- Cascading shadow maps
- Exponential height fog
- PBR
- Lights (spot/omni, multiple lights)
- More primitive shapes
- Meshes
- Textures

## Examples
[Playground](https://www.flecs.dev/explorer/?wasm=https://www.flecs.dev/explorer/playground.js)

![image](https://user-images.githubusercontent.com/9919222/226444497-7dd79478-8346-436e-8c89-eaf4ef52bdd7.png)

[City demo](https://flecs.dev/city)

<img width="1379" alt="Screen Shot 2023-03-20 at 12 25 03 PM" src="https://user-images.githubusercontent.com/9919222/226445050-8db1b453-9518-4418-9a48-f02115599c23.png">

[Tower defense demo](https://www.flecs.dev/tower_defense/etc/)

<img width="1398" alt="Screen Shot 2023-03-20 at 12 15 08 PM" src="https://user-images.githubusercontent.com/9919222/226444789-e9afab75-ee04-4db9-be39-39902707bbf6.png">
