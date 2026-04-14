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

#ifdef QS_PLATFORM_WINDOWS
#define QS_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define QS_PLUGIN_EXPORT extern "C"
#endif
