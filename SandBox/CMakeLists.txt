add_executable(SandBox WIN32 "SandBox.cpp"
                       "SandBox.hpp"
                       "Main.cpp")

target_link_libraries(SandBox Helios)

set_property(TARGET SandBox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
