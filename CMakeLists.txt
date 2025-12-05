cmake_minimum_required(VERSION 3.15)
project(SCS_CPU_Simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== SFML 3 Path =====
set(SFML_ROOT "C:/Users/Mihai/Documents/UTCN/SFML-3.0.2")

find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

# ===== ImGui + ImGui-SFML =====
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
add_subdirectory(external/imgui-sfml)

# ===== Source Files =====
# Automatically grab all .cpp files in src/ and src/CPU
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/CPU/*.cpp
)

# ===== Executable =====
add_executable(SCS_CPU_Simulator ${SOURCES})

# ===== Include Directories =====
target_include_directories(SCS_CPU_Simulator PRIVATE
    includes
    src
    src/CPU_Controller
    ${IMGUI_DIR}
)

# ===== Link Libraries =====
target_link_libraries(SCS_CPU_Simulator PRIVATE
    SFML::System
    SFML::Window
    SFML::Graphics
    ImGui-SFML::ImGui-SFML
)
