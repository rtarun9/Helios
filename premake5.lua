workspace "Helios"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release"
    }

    -- To decide if DXC and the DirectXAgility SDK need to be copied or not (default option is to copy it).
    -- When project has already been generated previously, these Folders were already copied. So, the next time copying them after each build would be unnecessary.
    -- do avoid copying, in command line specify '--copy_thirdparty_folders=No'.
    newoption 
    {
        trigger = "copy_thirdparty_folders",
        value = "Options",
        description = "Disable / Enable copying third party folders into appropirate path post build.",
        allowed = 
        {
            {"Yes", "Copy Files during each postbuild."},
            {"No", "Copying of Files will be skipped after each postbuild."}
        },
        default = "Yes"
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

    filter "options:copy_thirdparty_folders=Yes"
        prebuildcommands
        {
            "{COPYDIR} ThirdParty/DirectXShaderCompiler/bin/x64 Shaders",
            "{CHDIR} Shaders/",
            "call CompileShaders.bat",
            "{CHDIR} ../"
        }

        postbuildcommands 
        {
            "{COPYDIR} ThirdParty/DirectXAgilitySDK/build/native/bin/x64 Bin/%{cfg.buildcfg}/D3D12/",
        }

    filter "options:copy_thirdparty_folders=No"
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



