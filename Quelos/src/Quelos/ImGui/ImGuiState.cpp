#include "qspch.h"
#include "ImGuiState.h"

namespace Quelos {
    static ImGuiStateFactory s_StateFactory = nullptr;
    void ImGuiState::Register(const ImGuiStateFactory imGuiStateFactory) {
        s_StateFactory = imGuiStateFactory;
    }

    Ref<ImGuiState> ImGuiState::Create() {
        if (!s_StateFactory) {
            return nullptr;
        }

        return s_StateFactory();
    }
}
