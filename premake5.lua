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
    cppdialect "C++latest"

    targetdir "Bin/%{cfg.buildcfg}"
    objdir "Bin/Obj/%{cfg.buildcfg}"

    debugdir "."

    includedirs
    {
        "Engine",
        "SandBox",
        "ThirdParty",
        "Shaders"
    }

    files
    {
        "Engine/**.hpp",
        "Engine/**.cpp",
        "SandBox/**.hpp",   
        "SandBox/**.cpp",
        "Shaders/**.hlsl",
        "Shaders/**.hlsli",
    }

    pchsource "Engine/Pch.cpp"
    pchheader "Pch.hpp"

    staticruntime "off"
    runtime "Debug"
    systemversion "latest"

    prebuildcommands
    {
        "{CHDIR} Shaders/",
        "call CompileShaders.bat",
        "{CHDIR} .."
    }

    filter "configurations:Debug"
        defines "_DEBUG"
        optimize "Debug"

    filter "configurations:Release"
        optimize "Full"

    filter "files:**.hlsl"
        buildaction "None"

    filter "files:**.hlsli"
        buildaction "None"

    filter {}

        -- link system libs.
        libdirs {}

        links 
        {    
            "d3d12", 
            "dxgi",
            "dxguid", 
            "d3dcompiler",
            "user32",
            "gdi32",
            "ThirdParty"
        }

        -- link thirdparty libs.
        libdirs "Bin/%{cfg.buildcfg}"
        links "ThirdParty"
        
        -- run thirdparty premake scripts.
        group "ThirdParty"
        include "ThirdParty/premake.lua"



