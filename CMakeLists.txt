cmake_minimum_required(VERSION 3.28)
project(DataVisualization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# Fetch SFML
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

# Dear ImGui (must stay < 1.92.0 for current imgui-sfml)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

# Fetch imgui first so that imgui-sfml can find it
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to Dear ImGui" FORCE)

# Prevent imgui-sfml from trying to find another SFML
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

# ImGui-SFML
FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v3.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_MakeAvailable(SFML imgui-sfml)

# Recursively gather all .cpp files in the src directory when build
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(DataVisualization ${SRC_FILES})

# Showing the source files in the CMake output for debugging purposes
foreach(f ${SRC_FILES})
    message(STATUS "Source: ${f}")
endforeach()

target_include_directories(DataVisualization PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(DataVisualization PRIVATE cxx_std_17)

target_link_libraries(DataVisualization
    PRIVATE
        ImGui-SFML::ImGui-SFML
    SFML::Audio
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DataVisualization)

target_compile_options(DataVisualization PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FS>
)

target_compile_definitions(DataVisualization PRIVATE
    ASSET_FONT="${CMAKE_SOURCE_DIR}/fonts"
    ASSET_AUDIO="${CMAKE_SOURCE_DIR}/sounds"
)

