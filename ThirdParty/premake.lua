project "ThirdParty"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"

    targetdir "../Bin/%{cfg.buildcfg}"
    objdir "../Bin/Obj/%{cfg.buildcfg}"

    debugdir "."

    staticruntime "off"
    runtime "Debug" 
    systemversion "latest"

    files
    {
        "D3D12MemoryAllocator/D3D12MemAlloc.cpp",
        "D3D12MemoryAllocator/D3D12MemAlloc.h",
        "ImGUI/*.h", 
        "ImGUI/*.cpp",
        "TinyGLTF/tiny_gltf.cpp",
        "TinyGLTF/tiny_gltf.h",
        "STB/stb_image.h",
        "D3DX12/d3dx12.h"
    }