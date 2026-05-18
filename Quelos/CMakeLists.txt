cmake_minimum_required(VERSION 4.1.2)

function(add_quelos_module out_var path)
    file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/${path}/*.cpp
    )
    set(${out_var} ${SRC} PARENT_SCOPE)
    message(STATUS "Module ${path}: ${SRC}")
endfunction()

add_quelos_module(QUELOS_CORE_SRC src/Quelos/Core)
add_quelos_module(QUELOS_RENDERER_SRC src/Quelos/Renderer)
add_quelos_module(QUELOS_ASSET_MANAGER_SRC src/Quelos/AssetManager)
add_quelos_module(QUELOS_MATH_SRC src/Quelos/Math)
add_quelos_module(QUELOS_IMGUI_SRC src/Quelos/ImGui)
add_quelos_module(QUELOS_PROJECT_SRC src/Quelos/Project)
add_quelos_module(QUELOS_SCENES_SRC src/Quelos/Scenes)
add_quelos_module(QUELOS_SERIALIZATION_SRC src/Quelos/Serialization)
add_quelos_module(QUELOS_UTILITY_SRC src/Quelos/Utility)

if(WIN32)
    file(GLOB QUELOS_PLATFORM_SRC CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Windows/*.cpp
    )
elseif(APPLE)
    file(GLOB QUELOS_PLATFORM_SRC CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Mac/*.cpp
    )
elseif(UNIX)
    file(GLOB QUELOS_PLATFORM_SRC CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Linux/*.cpp
    )
endif()

set(QUELOS_SDL_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Window/SDLWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Window/imgui_impl_sdl3.cpp
)

set(QUELOS_GLFW_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Window/GLFWWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Quelos/Platform/Window/imgui_impl_glfw.cpp
)

set(IMGUI_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
)

function(setup_quelos_target target type)
    add_library(${target} ${type} EXCLUDE_FROM_ALL
            ${QUELOS_CORE_SRC}
            ${QUELOS_RENDERER_SRC}
            ${QUELOS_ASSET_MANAGER_SRC}
            ${QUELOS_MATH_SRC}
            ${IMGUI_SRC}
            ${QUELOS_IMGUI_SRC}
            ${QUELOS_PROJECT_SRC}
            ${QUELOS_SCENES_SRC}
            ${QUELOS_SERIALIZATION_SRC}
            ${QUELOS_UTILITY_SRC}
            ${QUELOS_PLATFORM_SRC}
            ${QUELOS_SDL_SRC}
    )

    target_include_directories(${target} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxhash>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/hlslpp/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/magic_enum>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/stduuid>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/unordered_dense>
    )

    target_precompile_headers(${target} PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/qspch.h>
    )

    target_compile_definitions(${target} PUBLIC
            GLM_ENABLE_EXPERIMENTAL
            GLM_FORCE_QUAT_DATA_WXYZ
            GLM_FORCE_LEFT_HANDED
            HLSLPP_FEATURE_TRANSFORM
    )

    target_compile_features(${target} PUBLIC cxx_std_23)

    if (MSVC)
        target_compile_options(${target} PUBLIC
                /Zc:preprocessor
                /Zc:__cplusplus
                /utf-8
        )
    endif()

    if (WIN32)
        target_compile_definitions(${target} PUBLIC QS_PLATFORM_WINDOWS)
    elseif (APPLE)
        target_compile_definitions(${target} PUBLIC QS_PLATFORM_MACOS)
    elseif (UNIX)
        target_compile_definitions(${target} PUBLIC QS_PLATFORM_LINUX)
    endif()

    target_link_libraries(${target}
            PRIVATE
            #glfw
            SDL3::SDL3-static
            libDaScript
            freetype
            stb_image
            TracyClient
            qxxhash
    )

    target_include_directories(${target}
            PUBLIC
            $<TARGET_PROPERTY:TracyClient,INTERFACE_INCLUDE_DIRECTORIES>
    )

    if(type STREQUAL "SHARED")
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

        target_compile_definitions(${target}
                PRIVATE QS_LIB_BUILD QS_LIB_SHARED
                PUBLIC  QS_LIB_SHARED
        )

        target_link_libraries(${target}
                PUBLIC
                flecs
        )

        install(TARGETS ${target}
                EXPORT QuelosTargets
                RUNTIME DESTINATION lib
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib
                COMPONENT QuelosSDK
        )

        install(DIRECTORY src/
                DESTINATION include
                COMPONENT QuelosSDK
        )

        install(EXPORT QuelosTargets
                FILE QuelosTargets.cmake
                NAMESPACE Quelos::
                DESTINATION cmake
                COMPONENT QuelosSDK
        )

        install(DIRECTORY vendor/hlslpp/include
                DESTINATION include/hlslpp/
                COMPONENT QuelosSDK
        )

        install(DIRECTORY vendor/spdlog/
                DESTINATION include/spdlog
                COMPONENT QuelosSDK
        )

        install(DIRECTORY vendor/magic_enum/
                DESTINATION include/magic_enum
                COMPONENT QuelosSDK
        )

        install(DIRECTORY vendor/stduuid/
                DESTINATION include/stduuid
                COMPONENT QuelosSDK
        )

        install(DIRECTORY vendor/unordered_dense/
                DESTINATION include/unordered_dense
                COMPONENT QuelosSDK
        )
    else()
        target_compile_definitions(${target}
                PRIVATE QS_LIB_BUILD QS_LIB_STATIC
                PUBLIC  QS_LIB_STATIC
        )

        target_link_libraries(${target}
                PUBLIC
                flecs_static
        )
    endif()
endfunction()

setup_quelos_target(QuelosStatic STATIC)
setup_quelos_target(QuelosShared SHARED)

add_library(Quelos::Static ALIAS QuelosStatic)
add_library(Quelos::Shared ALIAS QuelosShared)
