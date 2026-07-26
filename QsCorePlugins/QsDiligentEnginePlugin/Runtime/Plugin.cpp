#include "ImGui/DiligentImGuiState.hpp"
#include "Quelos/Plugin/PluginAPI.h"
#include "Quelos/Renderer/Renderer.h"

#include "Renderer/DiligentRendererContext.h"

using namespace Quelos;

QS_PLUGIN_EXPORT void RegisterDiligentEnginePlugin(PluginContext& pluginContext) {
    Renderer::RegisterRendererContext({
        [] {
            return SharedAs<RendererContext>(CreateShared<DiligentRendererContext>());
        },
        [] {
            return SharedAs<ImGuiState>(CreateShared<Diligent::DiligentImGuiState>());
        }
    });
}
