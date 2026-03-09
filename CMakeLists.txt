cmake_minimum_required(VERSION 4.1.2)

project(Quelos
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configurations
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_definitions(BX_CONFIG_DEBUG)

# MSVC-specific options
if (MSVC)
    add_compile_options(
        /Zc:preprocessor
        /Zc:__cplusplus
        /utf-8
    )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies
add_subdirectory(Quelos/vendor/bgfx.cmake)

add_subdirectory(Quelos/vendor/GLFW)
add_subdirectory(Quelos/vendor/SDL)
add_subdirectory(Quelos/vendor/imgui)
add_subdirectory(Quelos/vendor/flecs)

# Daslang options
set(DAS_GLFW_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_STDDLG_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_STBIMAGE_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_STBTRUETYPE_DISABLED ON CACHE BOOL "" FORCE)

set(DAS_TOOLS_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_TESTS_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_TUTORIAL_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_AOT_EXAMPLES_DISABLED ON CACHE BOOL "" FORCE)

add_subdirectory(Quelos/vendor/daScript)

# Header-only libraries
add_subdirectory(Quelos/vendor/glm)
add_subdirectory(Quelos/vendor/spdlog)
add_subdirectory(Quelos/vendor/stduuid)
add_subdirectory(Quelos/vendor/xxhash)
add_subdirectory(Quelos/vendor/magic_enum)
add_subdirectory(Quelos/vendor/ska)

if (MSVC)
    add_compile_options(
            /await:strict
    )
endif()

# Main projects
add_subdirectory(Quelos)
add_subdirectory(Quelos-Editor)
