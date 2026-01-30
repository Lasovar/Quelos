cmake_minimum_required(VERSION 3.20)

project(Quelos-Editor LANGUAGES CXX)

add_executable(Quelos-Editor)

file(GLOB_RECURSE QUELOS_EDITOR_SRC
    src/*.cpp
    src/*.h
)

target_sources(Quelos-Editor PRIVATE ${QUELOS_EDITOR_SRC})

add_custom_command(TARGET Quelos-Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Quelos-Editor/Assets
        $<TARGET_FILE_DIR:Quelos-Editor>/Assets
)

target_include_directories(Quelos-Editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Quelos-Editor
    PRIVATE
        Quelos
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
