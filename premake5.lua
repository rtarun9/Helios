workspace "Helios"
    architecture "x64"
    location "VsFiles"

    configurations
    {
        "Debug",
        "Release"
    }

project "Helios"
    kind "WindowedApp"

    language "C++"
    cppdialect "C++latest"

    targetdir "Bin/%{cfg.buildcfg}"
    objdir "Bin/Obj/%{cfg.buildcfg}"

    debugdir "."
    
    links 
    { 
        "d3d12", 
        "dxgi",
        "dxguid", 
        "d3dcompiler",
        "user32",
        "gdi32"
    }

    includedirs
    {
        "Engine",
        "SandBox",
        "Shaders"
    }

    files
    {
        "Engine/**.hpp",
        "Engine/**.cpp",
        "SandBox/**.hpp",   
        "SandBox/**.cpp",
        "Shaders/**.hlsl",
        "Shaders/**.hlsli"
    }

    pchsource "Engine/Pch.cpp"
    pchheader "Pch.hpp"

    staticruntime "off"
    runtime "Debug"
    systemversion "latest"

    prebuildcommands
    {
        "{COPYDIR} ../ThirdParty/DirectXShaderCompiler/bin/x64 ../Shaders",
        "{CHDIR} ../Shaders/",
        "call CompileShaders.bat",
        "{CHDIR} ../"
    }

    postbuildcommands 
    {
        "{COPYDIR} ../ThirdParty/DirectXAgilitySDK/build/native/bin/x64 ../Bin/%{cfg.buildcfg}/D3D12/",
    }

    filter "configurations:Debug"
        defines "_DEBUG"
        optimize "Debug"

    filter "configurations:Release"
        optimize "Full"

    filter "files:**.hlsl"
        buildaction "None"
