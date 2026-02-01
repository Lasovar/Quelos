#include "qspch.h"
#include "ImGuiState.h"

#include "Quelos/Platform/bgfx/ImGui/bgfxImGuiState.h"

namespace Quelos {
    Ref<ImGuiState> ImGuiState::Create() {
        return CreateRef<bgfxImGuiState>();
    }
}
