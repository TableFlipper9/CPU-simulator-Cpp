cmake_minimum_required(VERSION 3.15)
project(SCS_CPU_Simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_ROOT "C:/Users/Mihai/Documents/UTCN/SFML-3.0.2")

find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
add_subdirectory(external/imgui-sfml)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/CPU/*.cpp
    src/CPU/CPU_Controller/*.cpp
    src/CPU/CPU_Controller/Hazzard_handeling/*.cpp
)

add_executable(SCS_CPU_Simulator ${SOURCES})

target_include_directories(SCS_CPU_Simulator PRIVATE
    includes
    src
    src/CPU_Controller
    src/CPU/CPU_Controller/Hazzard_handeling
    ${IMGUI_DIR}
)

target_link_libraries(SCS_CPU_Simulator PRIVATE
    SFML::System
    SFML::Window
    SFML::Graphics
    ImGui-SFML::ImGui-SFML
)
