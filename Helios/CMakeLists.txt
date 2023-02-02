set(SRC_FILES
    "Source/Core/Application.cpp"
    "Include/Core/Application.hpp"

    "Include/Graphics/d3dx12.hpp"

    "Source/Graphics/GraphicsDevice.cpp"
    "Include/Graphics/GraphicsDevice.hpp"
    
    "Source/Graphics/CommandQueue.cpp"
    "Include/Graphics/CommandQueue.hpp"

    "Source/Graphics/Context.cpp"
    "Include/Graphics/Context.hpp"
)

add_library(Helios STATIC ${SRC_FILES})
target_include_directories(Helios PUBLIC "Include")
target_link_libraries(Helios PUBLIC External d3d12.lib)

# Setup precompiled headers.
target_precompile_headers(
    Helios
    PUBLIC
    "Include/Pch.hpp"
)
