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

    enum class SceneState {
        Edit = 0, Play
    };

    class EditorLayer : public Layer {
    public:
        explicit EditorLayer(const std::string& name = "Editor Layer")
            : Layer(name) {
        }

        void OnAttach() override;

        void Tick(float deltaTime) override;
        void ImGuiRender() override;
        void OnScenePlay();
        void OnSceneStop();
        void UI_Toolbar();
        void OnEvent(Event& event) override;

        void OpenSceneWorkspace(const AssetID& handle);

        static void RecompilerShader(const AssetRef<GraphicsShader>& shader) {
            s_ShaderRecompilationStack.push_back(shader);
        }

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
        static EditorLayer* s_Instance;

    private:
        SceneState m_SceneState = SceneState::Edit;
        bool m_ScenePaused = false;
        bool m_SceneStep = false;

        ProjectSerializer m_ProjectSerializer;

        Ref<Texture2D> m_IconPlay, m_IconPause, m_IconStop, m_IconStep, m_Container;

        Ref<Scene> m_EditorScene;
        Ref<Scene> m_ActiveScene;

        ImGuiWindowClass m_EditorLayerClass;
        UndoSystem m_UndoSystem{};

        Vec<Ref<Workspace>> m_Workspaces;
        ContentBrowserPanel m_ContentBrowserPanel;

        bool m_CtrlKey : 1 = false;
        bool m_ZKey : 1 = false;
        bool m_YKey : 1 = false;
        bool m_ShiftKey : 1 = false;

        // SHADER
        static HashMap<const char*, QS_ShaderCompiler> s_ShaderCompilers;
        static Vec<AssetRef<GraphicsShader>> s_ShaderRecompilationStack;
    };
}
