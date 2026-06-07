#pragma once

#include <Quelos/Core/Layer.h>
#include <Quelos/Scenes/Scene.h>
#include <Quelos/Core/Ref.h>

#include "ProjectSerializer.h"
#include "UndoSystem.h"
#include "Panels/ViewportPanel.h"
#include "QuelosEditor/EditorAPI.h"
#include "Workspaces/SceneWorkspace.h"

namespace QuelosEditor {
    extern QS_EditorAPI g_EditorAPI;

    using namespace Quelos;

    class EditorLayer : public Layer {
    public:
        explicit EditorLayer(const std::string& name = "Editor Layer")
            : Layer(name) {
        }

        void OnAttach() override;

        void Tick(float deltaTime) override;
        void ImGuiRender() override;
        void OnEvent(Event& event) override;

        void OpenAssetWorkspace(const AssetMetadata& metadata);

        static EditorLayer& Get() { return *s_Instance; }

        static void RegisterShaderCompiler(const char* rendererName, QS_ShaderCompiler compiler);

        static QS_ShaderCompiler GetShaderCompiler() {
            for (const auto& compiler : s_ShaderCompilers | std::views::values) {
                return compiler;
            }

            return QS_ShaderCompiler {
                .Compile = nullptr,
                .FreeBuffer = nullptr
            };
        }
    private:
        void FlushOpenAssetWorkspace();

    private:
        static EditorLayer* s_Instance;

    private:
        ProjectSerializer m_ProjectSerializer;

        Ref<EditorAssetManager> m_EditorAssetManager;

        ImGuiWindowClass m_EditorLayerClass;
        UndoSystem m_UndoSystem{};

        Vec<AssetMetadata> m_OpenAssetWorkspaceRequests;
        HashMap<AssetID, Scope<Workspace>> m_Workspaces;
        ContentBrowserPanel m_ContentBrowserPanel;

        HashMap<AssetTypeID, WorkspaceFactory> m_WorkspaceFactories;

        bool m_CtrlKey : 1 = false;
        bool m_ZKey : 1 = false;
        bool m_YKey : 1 = false;
        bool m_ShiftKey : 1 = false;

        // SHADER
        static HashMap<const char*, QS_ShaderCompiler> s_ShaderCompilers;
    };
}
