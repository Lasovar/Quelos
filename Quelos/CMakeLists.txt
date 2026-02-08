cmake_minimum_required(VERSION 3.20)

project(Quelos LANGUAGES CXX)

add_library(Quelos STATIC)

file(GLOB_RECURSE QUELOS_SRC
    src/*.cpp
    src/*.h
)

target_sources(Quelos PRIVATE ${QUELOS_SRC})

target_precompile_headers(Quelos
    PRIVATE
        src/qspch.h
)

target_include_directories(Quelos
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Quelos
    PUBLIC
        GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(Quelos
    PRIVATE
        glfw
        bimg
        bimg_decode
        bgfx
        bx
        SDL3::SDL3-static
        xxhash
    PUBLIC
        imgui
        stduuid
        flecs_static
        glm
        spdlog
)

set_target_properties(Quelos PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

if (WIN32)
    target_compile_definitions(Quelos PUBLIC QUELOS_PLATFORM_WINDOWS)
elseif (APPLE)
    target_compile_definitions(Quelos PUBLIC QUELOS_PLATFORM_MACOS)
elseif (UNIX)
    target_compile_definitions(Quelos PUBLIC QUELOS_PLATFORM_LINUX)

    #target_include_directories(Quelos PRIVATE ${GTK4_INCLUDE_DIRS})
endif()
