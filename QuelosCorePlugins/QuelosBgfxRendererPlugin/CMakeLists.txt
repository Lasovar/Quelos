cmake_minimum_required(VERSION 3.20)
project(QuelosBgfxRendererPlugin)

set(BGFX_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)
set(BGFX_BUILD_TOOLS ON)
set(BGFX_BUILD_TOOLS_SHADER ON)

add_subdirectory(vendor/bgfx.cmake)
add_subdirectory(vendor/tiny-process-library)

file(GLOB_RECURSE QUELOS_BGFX_SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.cpp
)

file(GLOB_RECURSE QUELOS_BGFX_EDITOR_SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.cpp
)

add_library(QuelosBgfxRendererPlugin SHARED ${QUELOS_BGFX_SRC} ${QUELOS_BGFX_EDITOR_SRC})

target_link_libraries(QuelosBgfxRendererPlugin PRIVATE
        Quelos::Shared
        bimg
        bimg_decode
        bgfx
        bx
        tiny-process-library
)

target_include_directories(QuelosBgfxRendererPlugin PRIVATE Runtime)


add_dependencies(QuelosBgfxRendererPlugin shaderc)

set(SHADERC_OUT_DIR "$<TARGET_FILE_DIR:QuelosBgfxRendererPlugin>/Tools/bin")

set_target_properties(shaderc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SHADERC_OUT_DIR}"
)

add_custom_command(TARGET QuelosBgfxRendererPlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${BGFX_DIR}/tools/bin
        $<TARGET_FILE_DIR:QuelosBgfxRendererPlugin>/Tools
)
