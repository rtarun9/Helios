set(SRC_FILES
    "core/Application.cpp"
)

add_library(Helios STATIC ${SRC_FILES})
target_include_directories(Helios PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Helios PUBLIC External)

# Setup precompiled headers.
target_precompile_headers(
    Helios
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include/Pch.hpp
)
