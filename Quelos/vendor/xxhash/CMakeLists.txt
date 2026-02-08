add_library(xxhash INTERFACE)

set_source_files_properties(
        xxhash.c
        PROPERTIES LANGUAGE C
)

target_sources(
        xxhash
        PRIVATE
        xxhash.c
)

target_include_directories(xxhash INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

