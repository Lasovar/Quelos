cmake_minimum_required(VERSION 4.1.2)

project(Quelos-Editor LANGUAGES CXX)

add_subdirectory(vendor/efsw)

add_executable(Quelos-Editor)

target_compile_definitions(QuelosShared PUBLIC QUELOS_USE_SDL)

file(GLOB_RECURSE QUELOS_EDITOR_SRC
    src/*.cpp
    src/*.h
)

target_sources(Quelos-Editor PRIVATE ${QUELOS_EDITOR_SRC})

#[[add_custom_command(TARGET Quelos-Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Quelos-Editor/SandboxProject
        $<TARGET_FILE_DIR:Quelos-Editor>/SandboxProject
)]]

target_include_directories(Quelos-Editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Quelos-Editor
    PRIVATE
        Quelos::Shared
        QsDiligentEnginePlugin
        rapidfuzz
        assimp
        efsw-static
)

include(ExternalProject)

set(TRACY_OUT_DIR "${CMAKE_BINARY_DIR}/Tools/bin")

#[[
ExternalProject_Add(tracy_profiler
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/Quelos/vendor/tracy/profiler

        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${TRACY_OUT_DIR}

        BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ""
)
]]


target_compile_definitions(QuelosShared PUBLIC QS_ENABLE_PROFILING)
target_compile_definitions(Quelos-Editor PUBLIC QS_ENABLE_PROFILING)

# Quelos Engine SDK
set(QUELOS_SDK_OUTPUT_DIR
        "${CMAKE_BINARY_DIR}/bin/EngineSDK"
)

add_custom_target(QuelosSDK
        COMMAND ${CMAKE_COMMAND}
        --install ${CMAKE_BINARY_DIR}
        --prefix ${QUELOS_SDK_OUTPUT_DIR}
        --component QuelosSDK
)

add_dependencies(QuelosSDK QuelosShared)
add_dependencies(Quelos-Editor QuelosSDK)

if (WIN32)
    target_compile_definitions(Quelos-Editor PRIVATE QUELOS_EDITOR_PLATFORM_WINDOWS)
elseif (APPLE)
    target_compile_definitions(Quelos-Editor PRIVATE QUELOS_EDITOR_PLATFORM_MACOS)
elseif (UNIX)
    target_compile_definitions(Quelos-Editor PRIVATE QUELOS_EDITOR_PLATFORM_LINUX)

#[[
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)

    target_include_directories(Quelos-Editor PRIVATE ${GTK4_INCLUDE_DIRS})
    target_link_libraries(Quelos-Editor PRIVATE ${GTK4_LIBRARIES})
    target_compile_options(Quelos-Editor PRIVATE ${GTK4_CFLAGS_OTHER})
]]
endif ()
