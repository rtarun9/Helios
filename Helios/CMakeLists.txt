set(SRC_FILES
    "Source/Core/Application.cpp"
    "Include/Core/Application.hpp"

    "Source/Core/ResourceManager.cpp"
    "Include/Core/ResourceManager.hpp"
    
    "Include/Graphics/d3dx12.hpp"

    "Source/Graphics/GraphicsDevice.cpp"
    "Include/Graphics/GraphicsDevice.hpp"
    
    "Source/Graphics/CommandQueue.cpp"
    "Include/Graphics/CommandQueue.hpp"

    "Source/Graphics/Context.cpp"
    "Include/Graphics/Context.hpp"

    "Source/Graphics/GraphicsContext.cpp"
    "Include/Graphics/GraphicsContext.hpp"

    "Source/Graphics/CopyContext.cpp"
    "Include/Graphics/CopyContext.hpp"

    "Source/Graphics/DescriptorHeap.cpp"
    "Include/Graphics/DescriptorHeap.hpp"
    
    "Source/Graphics/ShaderCompiler.cpp"
    "Include/Graphics/ShaderCompiler.hpp"

    "Source/Graphics/PipelineState.cpp"
    "Include/Graphics/PipelineState.hpp"

    "Source/Graphics/Resources.cpp"
    "Include/Graphics/Resources.hpp"

    "Source/Graphics/MemoryAllocator.cpp"
    "Include/Graphics/MemoryAllocator.hpp"
)

add_library(Helios STATIC ${SRC_FILES})
target_include_directories(Helios PUBLIC "Include")
target_link_libraries(Helios PUBLIC External d3d12.lib d3dcompiler.lib dxcompiler.lib)

# Setup precompiled headers.
target_precompile_headers(
    Helios
    PUBLIC
    "Include/Pch.hpp"
)
