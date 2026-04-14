cmake_minimum_required(VERSION 3.20)
project(QuelosBgfxRendererPlugin)

set(BGFX_LIBRARY_TYPE "STATIC" CACHE STRING "" FORCE)
set(BGFX_BUILD_TOOLS ON)
set(BGFX_BUILD_TOOLS_SHADER ON)

add_subdirectory(vendor/bgfx.cmake)

file(GLOB_RECURSE QUELOS_BGFX_SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.cpp
)

add_library(QuelosBgfxRendererPlugin SHARED ${QUELOS_BGFX_SRC})

target_link_libraries(QuelosBgfxRendererPlugin PRIVATE
        Quelos::Shared
        bimg
        bimg_decode
        bgfx
        bx
)

target_include_directories(QuelosBgfxRendererPlugin PRIVATE Runtime)

#[[

add_dependencies(Quelos-Editor shaderc)

set(SHADERC_OUT_DIR "$<TARGET_FILE_DIR:Quelos-Editor>/Tools/bin")

set_target_properties(shaderc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SHADERC_OUT_DIR}"
)


add_custom_command(TARGET Quelos-Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${BGFX_DIR}/tools/bin
        $<TARGET_FILE_DIR:Quelos-Editor>/Tools
)
]]
