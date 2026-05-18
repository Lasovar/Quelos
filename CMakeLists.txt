cmake_minimum_required(VERSION 4.1.2)

project(Quelos
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Configurations
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_definitions(BX_CONFIG_DEBUG)

# MSVC-specific options
if (MSVC)
    add_compile_definitions(QS_COMPILER_MSVC)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_definitions(QS_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_definitions(QS_COMPILER_CLANG)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies

# SDL configuration
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)

set(SDL_DBUS OFF CACHE BOOL "" FORCE)
set(SDL_IBUS OFF CACHE BOOL "" FORCE)
set(SDL_LIBUDEV OFF CACHE BOOL "" FORCE)

set(SDL_OPENGL ON CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)

add_subdirectory(Quelos/vendor/SDL)

add_subdirectory(Quelos/vendor/GLFW)
add_subdirectory(Quelos/vendor/tracy)
add_subdirectory(Quelos/vendor/freetype)
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

# Header libraries
add_subdirectory(Quelos/vendor/glm)
add_subdirectory(Quelos/vendor/hlslpp)
add_subdirectory(Quelos/vendor/spdlog)
add_subdirectory(Quelos/vendor/stduuid)
add_subdirectory(Quelos/vendor/magic_enum)
add_subdirectory(Quelos/vendor/ska)
add_subdirectory(Quelos/vendor/unordered_dense)
add_subdirectory(Quelos/vendor/stb_image)

if (MSVC)
    add_compile_options(/await:strict)
    add_compile_options(/permissive-)
endif()

# Core
add_subdirectory(Quelos)

# Editor
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
set(ASSIMP_BUILD_MINIZIP ON)
set(ASSIMP_HUNTER_ENABLED OFF)
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
add_subdirectory(Quelos-Editor/vendor/assimp)

add_subdirectory(Quelos-Editor/vendor/rapidfuzz-cpp)

add_subdirectory(QsCorePlugins/QsDiligentEnginePlugin)
add_subdirectory(Quelos-Editor)
