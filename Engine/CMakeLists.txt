set(SRC_FILES
    "Source/Core/Application.cpp"
    "Source/Core/Engine.cpp"
    "Source/Core/Timer.cpp"

    "Source/Editor/Editor.cpp"

    "Source/Graphics/API/CommandQueue.cpp"
    "Source/Graphics/API/ComputeContext.cpp"
    "Source/Graphics/API/Descriptor.cpp"
    "Source/Graphics/API/Device.cpp"
    "Source/Graphics/API/GraphicsContext.cpp"
    "Source/Graphics/API/MemoryAllocator.cpp"
    "Source/Graphics/API/MipMapGenerator.cpp"
    "Source/Graphics/API/PipelineState.cpp"
    "Source/Graphics/API/Resources.cpp"

    "Source/Graphics/RenderPass/DeferredGeometryPass.cpp"

    "Source/Scene/Camera.cpp"
    "Source/Scene/Light.cpp"
    "Source/Scene/Model.cpp"
    "Source/Scene/Scene.cpp"
    "Source/Scene/SkyBox.cpp"
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
