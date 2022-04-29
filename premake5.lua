workspace "Helios"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release"
    }

project "Helios"
    kind "WindowedApp"
    language "C++"
    targetdir "Bin/%{cfg.buildcfg}"

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
        "Engine/Graphics/d3dx12.h",      -- The extension has not been changed to .hpp, as this is a third party file which is included amongst the Engine's source file.
        "Engine/**.cpp",
        "SandBox/**.hpp",   
        "SandBox/**.cpp"
    }

    pchsource "Engine/Pch.cpp"
    pchheader "Pch.hpp"

    cppdialect "C++20"
    staticruntime "off"
    runtime "Debug"
    systemversion "latest"

    filter "configurations:Debug"
        defines "_DEBUG"
        optimize "On"

    filter "configurations:Release"
        optimize "On"

    postbuildcommands 
    {
        "copy ThirdParty\\DirectXAgilitySDK\\build\\native\\bin\\x64 Bin\\Debug\\D3D12\\",
    }