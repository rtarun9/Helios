# Helios

A Experimental, C++20 & DX12 renderer made for learning and trying out various graphics / rendering techniques.

# Features
* Bindless Rendering (Using SM 6.6's Resource / Sampler descriptor Heap).
* Normal Mapping.
* Diffuse and Specular IBL.
* Physically based rendering (PBR).
* Blinn-Phong Shading.
* Deferred Shading.
* HDR and Tone Mapping.
* Instanced rendering.
* Compute Shader mip map generation.
* Editor (ImGui Integration).

# Gallery
> PBR and IBL
![](Assets/Screenshots/IBL1.png)
![](Assets/Screenshots/IBL2.png)
![](Assets/Screenshots/IBL3.png)

> Deferred Shading
![](Assets/Screenshots/Deferred1.png)

> Editor (using ImGui)
![](Assets/Screenshots/Editor1.png)

# Building
+ This project uses VCPKG for package managment and premake as a build system.
+ After cloning the project, use the command `premake5.exe vs2022` to build (if vs2019 or other is used, replace vs2022 with that). 
+ If the output directory already contains the DirectX Agility SDK & DirectX Shader Compiler files, then you can use the flag `--copy_thirdparty_folders=No` to prevent repeatedly copying those files after each build.
+ Shaders are automatically compiled after build process.

