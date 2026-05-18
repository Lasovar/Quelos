add_library(qxxhash STATIC xxhash.c)

set_source_files_properties(
        xxhash.c
        PROPERTIES LANGUAGE C
)

target_include_directories(qxxhash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
