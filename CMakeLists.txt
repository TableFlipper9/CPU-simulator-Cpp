cmake_minimum_required(VERSION 3.15)
project(SCS_CPU_Simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_GUI_APP "Build SFML/ImGui GUI executable" ON)
option(BUILD_TESTS "Build CPU core tests" ON)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
if (BUILD_GUI_APP)
    # SFML / ImGui are only needed for the GUI
    find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)
    add_subdirectory(external/imgui-sfml)
endif()

file(GLOB_RECURSE CORE_SOURCES
    src/CPU/*.cpp
    src/CPU_Controller/*.cpp
    src/CPU_Controller/Hazzard_handeling/*.cpp
    src/CPU_Controller/Meta/*.cpp
)

add_library(cpu_core ${CORE_SOURCES})
target_include_directories(cpu_core PUBLIC
    includes
    src
    src/CPU_Controller
    src/CPU_Controller/Hazzard_handeling
)

if (BUILD_GUI_APP)
    file(GLOB_RECURSE GUI_SOURCES
        src/GUI/*.cpp
    )

    add_executable(SCS_CPU_Simulator src/main.cpp ${GUI_SOURCES})
    target_link_libraries(SCS_CPU_Simulator PRIVATE cpu_core)

    target_include_directories(SCS_CPU_Simulator PRIVATE
        ${IMGUI_DIR}
    )

    target_link_libraries(SCS_CPU_Simulator PRIVATE
        SFML::System
        SFML::Window
        SFML::Graphics
        ImGui-SFML::ImGui-SFML
    )
endif()

if (BUILD_TESTS)
    enable_testing()
    add_executable(cpu_tests tests/cpu_tests.cpp)
    target_link_libraries(cpu_tests PRIVATE cpu_core)
    add_test(NAME cpu_tests COMMAND cpu_tests)
endif()
