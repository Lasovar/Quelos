#pragma once

#include "Quelos/Core/Event.h"
#include "imgui.h"

namespace QuelosEditor {
    using namespace Quelos;

    class Workspace {
    public:
        Workspace() = delete;

        explicit Workspace(std::string  workspaceName);
        virtual ~Workspace() = default;

        void Focus() {
            m_FocusRequest = true;
            m_FocusRequestFrame = ImGui::GetFrameCount();
        }

        [[nodiscard]] bool IsOpen() const { return m_IsOpen; }

        void OnImGuiRender(ImGuiID dockspaceID);

        virtual void Tick(float deltaTime) = 0;
        virtual void WorkspaceContents() = 0;

        virtual void OnEvent(Event& event) = 0;
    protected:
        std::string m_WorkspaceName;
        ImGuiID m_WorkspaceID;
        ImGuiWindowClass m_WorkspaceClass;

        uint32_t m_FocusRequestFrame = 0;
        bool m_FocusRequest = false;
        bool m_IsOpen = true;
    };
}
