add_library(xxhash STATIC xxhash.c)

set_source_files_properties(
        xxhash.c
        PROPERTIES LANGUAGE C
)

target_include_directories(xxhash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
