#pragma once

namespace Quelos {
    class ImGuiState : public RefCounted {
    public:
        virtual void Init(float fontSize) = 0;
        virtual void Destroy() = 0;

        virtual void BeginFrame(uint32_t viewId) = 0;
        virtual void EndFrame() = 0;
    public:
        static Ref<ImGuiState> Create();
    };
}
