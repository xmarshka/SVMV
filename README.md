## SVMV - Simple Vulkan Material Visualization

---

A glTF model viewer focused on rendering properties of the glTF PBR material model. Written in C++ using the Vulkan API and targeting the Microsoft Windows operating system (version 10+).

![Rendered "Corset" model](/screenshots/corset_full.PNG "Rendered "Corset" model")

The application allows the user to load glTF file containing models intended for PBR and see the rendered model lit by a monochrome ambient light and three orbiting point lights.

The shading model of the renderer adheres to the [glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) and is based on the metallic-roughness model. Based on the Cook-Torrance model, its components are:

 - Trowbridge/Reitz normal distribution function
 - Smith-GGX geometry function
 - Schlick-GGX fresnel function

 Supported material properties are:

 - Base color texture (also known as albedo)
 - Normal texture
 - Metallic-roughness texture
 - Occlusion texture
 - Emissive texture
  
Along with their respective factor properties.

## Features

---

 - Real-time rendering of glTF PBR models
 - Tangent generation using the MikkTSpace algorithm
 - Control over the position and color of three orbiting point lights in real time
 - Control over a free moving FPS-like camera

## Libraries used

---

 - Vulkan API via the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
 - shaderc via the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
 - Vulkan Memory Allocator via the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
 - [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)
 - [GLFW](https://www.glfw.org/)
 - [glm](https://github.com/g-truc/glm)
 - [tiny-gltf](https://github.com/syoyo/tinygltf)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)
 - [MikkTSpace](https://github.com/mmikk/MikkTSpace)

## Pre-built binaries

---

Included in the Releases section is a prebuilt executable of the application, along with a handful of sample models taken from the [sample model collection provided by the Khronos Group](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0):

 - [Antique Camera](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/AntiqueCamera)
 - [Corset](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Corset)
 - [Damaged Helmet](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/DamagedHelmet) by theblueturtle_
 - [Water Bottle](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/WaterBottle)

A batch file exists for each of the above mentioned models which runs the application and loads each of the models.

## Building

---

Necesarry to build the application are the above mentioned libraries, a Windows 10/11 operating system, a GPU that supports Vulkan and the CMake build system version 3.25+.

1. Clone the repository
2. Generate build files using CMake (ensure all necessary libraries are acquired)
3. Open the generated build files and compile the binaries

---

![Rendered "Damaged Helmet" model](/screenshots/helmet_full.PNG "Rendered "Damaged Helmet" model")
![Rendered "Antique Camera" model](/screenshots/camera_full.PNG "Rendered "Antique Camera" model")