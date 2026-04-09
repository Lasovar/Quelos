add_library(stb_image STATIC stb_image.cpp)

target_include_directories(stb_image PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(stb_image PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
