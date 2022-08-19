include(FetchContent)

FetchContent_Declare(
    D3D12MemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
    GIT_TAG f0458801c428cac151472d726a2999e21917de15
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG 8cbd391f096b9314a08670052cc0025cbcadb249
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a
)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        544969b7324cd6bba29f6203c7d78c7ea92dbab0
)

FetchContent_MakeAvailable(tinygltf D3D12MemoryAllocator stb)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    add_library(libimgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)

    target_include_directories(libimgui PUBLIC 
        ${imgui_SOURCE_DIR} 
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include)
endif()

FetchContent_GetProperties(D3D12MemoryAllocator)
target_include_directories(D3D12MemoryAllocator PUBLIC ${D3D12MemoryAllocator_SOURCE_DIR}/include "." ${CMAKE_SOURCE_DIR}/Shaders)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

add_library(ThirdParty INTERFACE)
target_link_libraries(ThirdParty INTERFACE tinygltf stb D3D12MemoryAllocator libimgui)