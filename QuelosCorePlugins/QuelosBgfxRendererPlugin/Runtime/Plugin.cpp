#include "Quelos/Plugin/PluginAPI.h"
#include "Quelos/Renderer/Renderer.h"

#include "Renderer/bgfxRendererContext.h"
#include "ImGui/bgfxImGuiState.h"

#include "Quelos/Core/Log.h"

QS_PLUGIN_EXPORT void RegisterBgfxRendererPlugin(Quelos::PluginContext& pluginContext) {
    Quelos::Renderer::RegisterRendererContext({
        [] {
            return Quelos::RefAs<Quelos::RendererContext>(Quelos::CreateRef<Quelos::bgfxRendererContext>());
        },
        [] {
            return Quelos::RefAs<Quelos::ImGuiState>(Quelos::CreateRef<Quelos::bgfxImGuiState>());
        }
    });
}
