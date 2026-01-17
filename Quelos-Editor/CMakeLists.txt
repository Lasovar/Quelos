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
