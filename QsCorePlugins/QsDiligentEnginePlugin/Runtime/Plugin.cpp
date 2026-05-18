#include "ImGui/ImGuiImplDiligent.hpp"
#include "Quelos/Plugin/PluginAPI.h"
#include "Quelos/Renderer/Renderer.h"

#include "Renderer/DiligentRendererContext.h"

using namespace Quelos;

QS_PLUGIN_EXPORT void RegisterDiligentEnginePlugin(PluginContext& pluginContext) {
    Renderer::RegisterRendererContext({
        [] {
            return RefAs<RendererContext>(CreateRef<DiligentRendererContext>());
        },
        [] {
            return RefAs<ImGuiState>(CreateRef<Diligent::DiligentImGuiState>());
        }
    });
}
