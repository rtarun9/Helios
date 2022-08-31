set(SRC_FILES
    "Source/Core/Log.cpp"
    "Source/Core/Application.cpp"
    "Source/Core/Engine.cpp"
    "Source/Core/Timer.cpp"

    "Source/Utility/Helpers.hpp"
    "Source/Utility/ResourceManager.hpp"

    "Source/Editor/Editor.cpp"

    "Source/Graphics/API/CommandQueue.cpp"
    "Source/Graphics/API/Context.hpp"
    "Source/Graphics/API/ComputeContext.cpp"
    "Source/Graphics/API/Descriptor.cpp"
    "Source/Graphics/API/Device.cpp"
    "Source/Graphics/API/GraphicsContext.cpp"
    "Source/Graphics/API/MemoryAllocator.cpp"
    "Source/Graphics/API/MipMapGenerator.cpp"
    "Source/Graphics/API/PipelineState.cpp"
    "Source/Graphics/API/Resources.cpp"

    "Source/Graphics/RenderPass/DeferredGeometryPass.cpp"
    "Source/Graphics/RenderPass/ShadowPass.cpp"
    "Source/Graphics/RenderPass/BloomPass.cpp"

    "Source/Scene/Camera.cpp"
    "Source/Scene/Light.cpp"
    "Source/Scene/Model.cpp"
    "Source/Scene/Scene.cpp"
    "Source/Scene/SkyBox.cpp"

    "Source/Core/Log.hpp"
    "Source/Core/Application.hpp"
    "Source/Core/Engine.hpp"
    "Source/Core/Timer.hpp"

    "Source/Utility/ResourceManager.cpp"

    "Source/Editor/Editor.hpp"

    "Source/Graphics/API/CommandQueue.hpp"
    "Source/Graphics/API/Context.cpp"
    "Source/Graphics/API/ComputeContext.hpp"
    "Source/Graphics/API/Descriptor.hpp"
    "Source/Graphics/API/Device.hpp"
    "Source/Graphics/API/GraphicsContext.hpp"
    "Source/Graphics/API/MemoryAllocator.hpp"
    "Source/Graphics/API/MipMapGenerator.hpp"
    "Source/Graphics/API/PipelineState.hpp"
    "Source/Graphics/API/Resources.hpp"

    "Source/Graphics/RenderPass/DeferredGeometryPass.hpp"
    "Source/Graphics/RenderPass/ShadowPass.hpp"
    "Source/Graphics/RenderPass/BloomPass.hpp"

    "Source/Scene/Camera.hpp"
    "Source/Scene/Light.hpp"
    "Source/Scene/Model.hpp"
    "Source/Scene/Scene.hpp"
    "Source/Scene/SkyBox.hpp"
)

add_subdirectory(ThirdParty)

add_library(Helios STATIC ${SRC_FILES})
target_include_directories(Helios PUBLIC Source)
target_link_libraries(Helios PUBLIC ThirdParty)

target_precompile_headers(
    Helios
    PUBLIC
    Source/Pch.hpp
)

add_custom_command(TARGET Helios POST_BUILD COMMAND cmd  "${CMAKE_SOURCE_DIR}//Shaders//CompileShaders.bat")