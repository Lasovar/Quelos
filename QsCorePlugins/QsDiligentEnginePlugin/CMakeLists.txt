cmake_minimum_required(VERSION 3.20)
project(QsDiligentEnginePlugin)

if(APPLE)
    enable_language(OBJC)
endif()

add_subdirectory(vendor/DiligentEngine)

file(GLOB_RECURSE QS_DILIGENT_SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.cpp
)

file(GLOB_RECURSE QS_DILIGENT_EDITOR_SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Editor/*.cpp
)

add_library(QsDiligentEnginePlugin SHARED ${QS_DILIGENT_SRC} ${QS_DILIGENT_EDITOR_SRC})

target_link_libraries(QsDiligentEnginePlugin PRIVATE
        Quelos::Shared
        Diligent-Common
        Diligent-GraphicsAccessories
)

target_include_directories(QsDiligentEnginePlugin PRIVATE Runtime)

target_compile_definitions(QsDiligentEnginePlugin PRIVATE QS_PLUGIN_LIB_BUILD)
target_compile_options(QsDiligentEnginePlugin PRIVATE -DENGINE_DLL)

target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-BuildSettings)

if(WIN32)
    if (D3D12_SUPPORTED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineD3D12-static)
    endif()
    if (D3D11_SUPPORTED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineD3D11-static)
    endif()
    if (VULKAN_SUPPORTED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineVk-static Vulkan-Headers)
    endif()
    if (GL_SUPPORTED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineOpenGL-static)
    endif()
elseif (UNIX)
    if(GL_SUPPORTED)
        find_package(OpenGL REQUIRED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineOpenGL-static OpenGL::GL)
    endif()

    if(VULKAN_SUPPORTED)
        target_link_libraries(QsDiligentEnginePlugin PRIVATE Diligent-GraphicsEngineVk-static)
    endif()
endif()
