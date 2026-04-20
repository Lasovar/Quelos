#pragma once

namespace Quelos {
    struct PluginContext {
        // expose what plugins need
        // e.g:
        // AssetManager*
        // Renderer registry
    };

    using RegisterPluginFn = void(*)(PluginContext&);
}


#if defined(_WIN32)
    #if defined(QS_LIB_SHARED)
        #if defined(QS_PLUGIN_LIB_BUILD)
            #define QS_PLUGIN_API __declspec(dllexport)
        #else
            #define QS_PLUGIN_API __declspec(dllimport)
        #endif
    #else
        #define QS_PLUGIN_API
    #endif
#else
    #if defined(QS__PLUGIN_LIB_SHARED)
        #define QS_PLUGIN_API __attribute__((visibility("default")))
    #else
        #define QS_PLUGIN_API
    #endif
#endif

#ifdef QS_PLATFORM_WINDOWS
#define QS_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define QS_PLUGIN_EXPORT extern "C"
#endif
