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

include(ExternalProject)


# Platform-specific binary name
if(WIN32)
    set(TRACY_BINARY_NAME "tracy-profiler.exe")
else()
    set(TRACY_BINARY_NAME "tracy-profiler")
endif()

set(TRACY_OUT_DIR "${CMAKE_BINARY_DIR}/tracy_profiler_build")

ExternalProject_Add(tracy_profiler
    SOURCE_DIR      ${CMAKE_SOURCE_DIR}/Quelos/vendor/tracy/profiler
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${TRACY_OUT_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${TRACY_OUT_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${TRACY_OUT_DIR}
        -DCPM_SOURCE_CACHE=${CMAKE_SOURCE_DIR}/Quelos/vendor/tracy/.cpm-cache
    BUILD_COMMAND   ${CMAKE_COMMAND} --build . --config Release
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${TRACY_OUT_DIR}/${TRACY_BINARY_NAME}"
)

add_dependencies(Quelos-Editor tracy_profiler)

# Copy to Tools/bin after build
add_custom_command(
        TARGET tracy_profiler
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Quelos-Editor>/Tools/bin"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${TRACY_OUT_DIR}/${TRACY_BINARY_NAME}"
        "$<TARGET_FILE_DIR:Quelos-Editor>/Tools/bin"

        COMMAND ${CMAKE_COMMAND} -E echo
        "Copying from: ${TRACY_OUT_DIR}/${TRACY_BINARY_NAME} to $<TARGET_FILE_DIR:Quelos-Editor>/Tools/bin"
)

set(SLANG_BUILD_DIR
        ${CMAKE_BINARY_DIR}/slang-build
)

if(CMAKE_CONFIGURATION_TYPES)
    set(SLANG_CONFIG_DIR "$<CONFIG>")
else()
    set(SLANG_CONFIG_DIR "Release")
endif()

set(SLANG_LIB_DIR "${SLANG_BUILD_DIR}/${SLANG_CONFIG_DIR}/lib")

if(WIN32)
    set(SLANG_RUNTIME_DIR "${SLANG_BUILD_DIR}/${SLANG_CONFIG_DIR}/bin")
    set(SLANG_VERSION "")
else()
    set(SLANG_RUNTIME_DIR "${SLANG_BUILD_DIR}/${SLANG_CONFIG_DIR}/lib")
    set(SLANG_VERSION "-2026.9")
endif()

set(SLANG_SLANG_COMPILER ${SLANG_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}slang-compiler${CMAKE_SHARED_LIBRARY_SUFFIX})
set(SLANG_SLANG_COMPILER_IMPLIB ${SLANG_LIB_DIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}slang-compiler${CMAKE_IMPORT_LIBRARY_SUFFIX})
set(SLANG_COMPILER_CORE ${SLANG_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}compiler-core${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SLANG_CORE ${SLANG_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}core${CMAKE_STATIC_LIBRARY_SUFFIX})

add_library(slang-compiler SHARED IMPORTED GLOBAL)
add_library(compiler-core STATIC IMPORTED GLOBAL)
add_library(core STATIC IMPORTED GLOBAL)
add_library(miniz STATIC IMPORTED GLOBAL)
add_library(lz4 STATIC IMPORTED GLOBAL)
add_library(cmark STATIC IMPORTED GLOBAL)

if (MSVC)
    set(SLANG_CMAKE_FLAGS "/Wv:18")
endif()

if(NOT EXISTS "${SLANG_SLANG_COMPILER}")
    ExternalProject_Add(
            slang_external

            SOURCE_DIR
            ${CMAKE_SOURCE_DIR}/Quelos-Editor/vendor/slang

            BINARY_DIR
            ${SLANG_BUILD_DIR}

            INSTALL_COMMAND ""

            CMAKE_GENERATOR
            Ninja

            CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_CXX_FLAGS=${SLANG_CMAKE_FLAGS}

            -DSLANG_ENABLE_FULL_IR_VALIDATION=OFF
            -DSLANG_ENABLE_IR_BREAK_ALLOC=OFF
            -DSLANG_ENABLE_ASAN=OFF

            -DSLANG_ENABLE_GFX=OFF
            -DSLANG_ENABLE_FULL_IR_VALIDATION=OFF
            -DSLANG_ENABLE_IR_BREAK_ALLOC=OFF
            -DSLANG_ENABLE_ASAN=OFF
            -DSLANG_ENABLE_TESTS=OFF
            -DSLANG_ENABLE_EXAMPLES=OFF
            -DSLANG_ENABLE_SLANGD=OFF
            -DSLANG_ENABLE_SLANGI=OFF
            -DSLANG_ENABLE_SLANGRT=OFF
            -DSLANG_ENABLE_REPLAYER=OFF
            -DSLANG_ENABLE_SLANG_GLSLANG=ON
            -DSLANG_ENABLE_DXIL=OFF
            -DSLANG_SLANG_LLVM_FLAVOR=DISABLE
            -DSLANG_LIB_TYPE=SHARED

            BUILD_BYPRODUCTS
            ${SLANG_SLANG_COMPILER}
            ${SLANG_SLANG_COMPILER_IMPLIB}
            ${SLANG_COMPILER_CORE}
            ${SLANG_CORE}
            ${SLANG_BUILD_DIR}/external/lz4/build/cmake/${CMAKE_STATIC_LIBRARY_PREFIX}lz4${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${SLANG_BUILD_DIR}/external/miniz/${CMAKE_STATIC_LIBRARY_PREFIX}miniz${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${SLANG_BUILD_DIR}/external/cmark/src/${CMAKE_STATIC_LIBRARY_PREFIX}cmark-gfm${CMAKE_STATIC_LIBRARY_SUFFIX}

            BUILD_ALWAYS OFF
            UPDATE_COMMAND ""
            UPDATE_DISCONNECTED ON
            CONFIGURE_HANDLED_BY_BUILD ON

            BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --parallel ${NPROCS}
    )

    add_dependencies(slang-compiler slang_external)
    add_dependencies(compiler-core slang_external)
    add_dependencies(core slang_external)
    add_dependencies(miniz slang_external)
    add_dependencies(lz4 slang_external)
    add_dependencies(cmark slang_external)
else()
    message(STATUS "Slang already built, skipping ExternalProject")
endif()

set(SLANG_RUNTIME_NAMES
        slang-compiler
        slang-glslang${SLANG_VERSION}
        slang-glsl-module${SLANG_VERSION}
)

set(SLANG_DEPLOY_LIBS "")

foreach(lib IN LISTS SLANG_RUNTIME_NAMES)
    list(APPEND SLANG_DEPLOY_LIBS "${SLANG_RUNTIME_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endforeach()

add_custom_command(TARGET Quelos-Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SLANG_DEPLOY_LIBS}
        $<TARGET_FILE_DIR:Quelos-Editor>
        COMMENT "Deploying Slang runtime libraries..."
)

set_target_properties(slang-compiler PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_SLANG_COMPILER}"
        IMPORTED_IMPLIB
        "${SLANG_SLANG_COMPILER_IMPLIB}"

        INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/Quelos-Editor/vendor/slang/include"
)

set_target_properties(compiler-core PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_COMPILER_CORE}"
)

set_target_properties(core PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_CORE}"
)

set_target_properties(miniz PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_BUILD_DIR}/external/miniz/${CMAKE_STATIC_LIBRARY_PREFIX}miniz${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

set_target_properties(lz4 PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_BUILD_DIR}/external/lz4/build/cmake/${CMAKE_STATIC_LIBRARY_PREFIX}lz4${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

set_target_properties(cmark PROPERTIES
        IMPORTED_LOCATION
        "${SLANG_BUILD_DIR}/external/cmark/src/${CMAKE_STATIC_LIBRARY_PREFIX}cmark-gfm${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

add_library(slang INTERFACE)

target_link_libraries(slang INTERFACE
        slang-compiler
        compiler-core
        core
        miniz
        lz4
        cmark
)

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
add_dependencies(Quelos-Editor QuelosSDK slang)

target_link_libraries(Quelos-Editor
        PRIVATE
        Quelos::Shared
        QsDiligentEnginePlugin
        rapidfuzz
        assimp
        efsw-static
        slang
)

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
