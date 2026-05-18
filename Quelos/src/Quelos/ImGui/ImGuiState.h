#pragma once
#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    class ImGuiState;

    class QS_API ImGuiState {
    public:
        virtual ~ImGuiState() = default;

        virtual void Init() = 0;
        virtual void Destroy() = 0;

        virtual void BeginFrame(uint32_t viewId) = 0;
        virtual void EndFrame() = 0;

    public:
        static void Register(ImGuiStateFactory imGuiStateFactory);
        static Ref<ImGuiState> Create();
    };
}
