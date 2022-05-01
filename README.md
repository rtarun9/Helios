# Helios

A work in progress, C++20 & DX12 renderer made for learning purposes.

# Features
* Bindless Rendering (Using SM 6.6's Resource Descriptor Heap).
* Normal Mapping.
* Diffuse and Specular IBL.

# Gallery
![](Assets/Screenshots/IBL1.png)
![](Assets/Screenshots/IBL2.png)

# Building
+ This project uses VCPKG for package managment and premake as a build system.
+ After cloning the project, use the command `premake5.exe vs2022` to build (if vs2019 or other is used, replace vs2022 with that). 
+ If the output directory already contains the DirectX Agility SDK & DirectX Shader Compiler files, then you can use the flag `--copy_thirdparty_folders=No` to prevent repeatedly copying those files after each build.
+ Shaders are automatically compiled after build process.