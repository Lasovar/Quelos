cmake_minimum_required(VERSION 3.20)

project(Quelos LANGUAGES CXX)

add_library(Quelos STATIC
        src/Quelos/Core/Events/WindowEvents.h
        src/Quelos/Core/Timer.h)

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
    PUBLIC
        bgfx
        bimg
        bx
        glfw
        imgui
        flecs
        glm
        spdlog
)

set_target_properties(Quelos PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

if (WIN32)
    target_compile_definitions(Quelos PRIVATE QUELOS_PLATFORM_WINDOWS)
elseif (APPLE)
    target_compile_definitions(Quelos PRIVATE QUELOS_PLATFORM_MACOS)
elseif (UNIX)
    target_compile_definitions(Quelos PRIVATE QUELOS_PLATFORM_LINUX)
endif()
