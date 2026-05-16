#include "EditorLayer.h"
#include <imgui.h>

#include "Quelos/Core/Base.h"

#include <Quelos/Scenes/Components.h>
#include <Quelos/Core/Log.h>

#include "imgui_internal.h"
#include "ProjectSerializer.h"
#include "AssetManagement/AssetImporters/SceneImporter.h"
#include "AssetManagement/AssetImporters/ShaderImporter.h"
#include "Quelos/ImGui/widgets/texture.h"
#include "Quelos/Core/Application.h"

#include "Quelos/Renderer/Shader.h"
#include "Quelos/Renderer/VertexBuffer.h"
#include "Quelos/Renderer/Material.h"
#include "Quelos/Scenes/ComponentRegistery.h"

namespace QuelosEditor {
    static std::vector<PosColorVertex> cubeVertices = {
        {-1.0f, 1.0f, 1.0f, 0xff000000},
        {1.0f, 1.0f, 1.0f, 0xff0000ff},
        {-1.0f, -1.0f, 1.0f, 0xff00ff00},
        {1.0f, -1.0f, 1.0f, 0xff00ffff},
        {-1.0f, 1.0f, -1.0f, 0xffff0000},
        {1.0f, 1.0f, -1.0f, 0xffff00ff},
        {-1.0f, -1.0f, -1.0f, 0xffffff00},
        {1.0f, -1.0f, -1.0f, 0xffffffff},
    };

    static const std::vector<uint16_t> cubeTriList = {
        0, 1, 2,
        1, 3, 2,
        4, 6, 5,
        5, 6, 7,
        0, 2, 4,
        4, 2, 6,
        1, 5, 3,
        5, 7, 3,
        0, 4, 1,
        4, 5, 1,
        2, 3, 6,
        6, 3, 7,
    };

    struct CubePlayer {
        float Speed = 2.0f;
        float Timer = 0.0f;
    };

    EditorLayer* EditorLayer::s_Instance = nullptr;
    HashMap<const char*, QS_ShaderCompiler> EditorLayer::s_ShaderCompilers;
    Vec<AssetRef<Shader>> EditorLayer::s_ShaderRecompilationStack;

    void EditorLayer::OnAttach() {
        s_Instance = this;

        m_ProjectSerializer = ProjectSerializer(Application::Get().GetApplicationPath() / "../../Quelos-Editor/SandboxProject");

        /*m_DefaultScene->GetWorld().each<CameraComponent>([](CameraComponent& cameraComponent) {
            cameraComponent.Camera.SetOrthographic(15, -100, 100);
        });*/

        /*auto m_DefaultScene = CreateRef<Scene>("TestScene");
        const Actor camera = m_DefaultScene->CreateActor("Camera");
        camera.Set(CameraComponent{SceneCamera()});
        camera.Set(LocalTransform{float3(0.0f, 0.0f, -15.0f), glm::identity<glm::quat>()});

        MeshComponent cubeMesh;
        cubeMesh.MeshData = CreateRef<Mesh>(cubeVertices, cubeTriList);
        cubeMesh.MaterialData = CreateRef<Material>("vs_cubes.bin", "fs_cubes.bin");

        const Actor floor = m_DefaultScene->CreateActor("Floor");
        floor.Set(LocalTransform{glm::vec3(0, 0, 0), glm::identity<glm::quat>(), glm::vec3(5, 0.5f, 5)});
        floor.Set(cubeMesh);

        const Actor cube = m_DefaultScene->CreateActor("Cube");
        cube.Set(LocalTransform{glm::vec3(-2.5f, 2.5f, 0), glm::identity<glm::quat>(), glm::vec3(1.0f)});
        cube.Set(cubeMesh);
        cube.Set(CubePlayer());
        cube.SetParent(floor);

        const Actor cube2 = m_DefaultScene->CreateActor("Cube2");
        cube2.Set(LocalTransform{glm::vec3(5.f, 2.5f, 0), glm::identity<glm::quat>(), glm::vec3(1.0f)});
        cube2.Set(cubeMesh);
        cube2.Set(CubePlayer{-2});
        cube2.SetParent(cube);

        const Actor cube3 = m_DefaultScene->CreateActor("Cube3");
        cube3.Set(LocalTransform{glm::vec3(0, 5, 0), glm::identity<glm::quat>(), glm::vec3(1.0f)});
        cube3.Set(cubeMesh);
        cube3.Set(CubePlayer{-10});
        cube3.SetParent(floor);

        const Actor cube4 = m_DefaultScene->CreateActor("Cube4");
        cube4.Set(LocalTransform{glm::vec3(0, 5, 0)});
        cube4.Set(cubeMesh);
        cube4.Set(CubePlayer{-10});
        cube4.SetParent(cube);

        SceneSerializer serializer(m_DefaultScene, "Assets/TestScene");
        serializer.BakePatches();*/

        /*m_DefaultScene->System<TransformComponent, CubePlayer>(
            [](const flecs::iter& it, size_t, TransformComponent& transform, CubePlayer& player) {
                player.Timer += it.delta_time();
                transform.Rotation = glm::quat({
                    player.Timer * player.Speed,
                    player.Timer * player.Speed,
                    0
                });
            }, "RotatePlayer");*/

        const uint64_t texture2DType = Hash::Fnv1a64("Quelos.Texture2D");
        const uint64_t textureType = Hash::Fnv1a64(TypeNameDisplay<Texture2D>());

        AssetRef<Shader> compiledShader = AssetRef<Shader>(AssetID("af5fda92-37f9-42e3-a189-3a5388090a14"));

        m_EditorLayerClass.ClassId = ImHashStr("EditorLayer");
        m_EditorLayerClass.DockingAllowUnclassed = false;

        m_ContentBrowserPanel.Init();
    }

    void EditorLayer::RegisterShaderCompiler(const char* rendererName, const QS_ShaderCompiler compiler) {
        s_ShaderCompilers[rendererName] = compiler;
    }

    void EditorLayer::Tick(const float deltaTime) {
        for (auto& shader : s_ShaderRecompilationStack) {
            ShaderImporter::RecompileShader(shader.TryGet());
        }

        s_ShaderRecompilationStack.clear();

        for (const auto& workspace : m_Workspaces) {
            workspace->Tick(deltaTime);
        }
    }

    void EditorLayer::ImGuiRender() {
        /*if (ImGui::Begin("Camera")) {
            CRef<TransformComponent> transform = s_Camera.GetRef<TransformComponent>();
            CRef<CameraComponent> camera = s_Camera.GetRef<CameraComponent>();
            ImGui::DragFloat3("Position", glm::value_ptr(transform->Position));
            glm::vec3 currentRot = glm::degrees(glm::eulerAngles(transform->Rotation));
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(currentRot))) {
                transform->Rotation = glm::radians(currentRot);
            }
        }
        ImGui::End();*/

        static ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_NoSplit |
            ImGuiDockNodeFlags_NoResize |
            ImGuiDockNodeFlags_NoDockingOverOther;

        static ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

        ImGui::Begin("##EditorDockspace", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        const ImGuiID globalDockspaceID = ImGui::GetID("EditorLayer_Dockspace");
        ImGui::DockSpace(globalDockspaceID, ImVec2(0, 0), dockspace_flags, &m_EditorLayerClass);

        static bool opt_fullscreen = true;
        static bool opt_padding = false;

        if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID)) {
            // If the only one window... no undocking
            if (node->Windows.Size == 1) {
                node->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
            }
            else {
                node->LocalFlags &= ~ImGuiDockNodeFlags_NoUndocking;
            }
        }

        // Show demo options and help
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", nullptr, &opt_fullscreen);
                ImGui::MenuItem("Padding", nullptr, &opt_padding);
                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoDockingOverCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode;
                }
                if (ImGui::MenuItem("Flag: NoDockingSplit", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit;
                }
                if (ImGui::MenuItem("Flag: NoUndocking", "", (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking;
                }
                if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                }
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                }
                if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) {
                    dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                }
                ImGui::Separator();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted(
                    "This demo has nothing to do with enabling docking!" "\n"
                    "This demo only demonstrate the use of ImGui::DockSpace() which allows you to manually\ncreate a docking node _within_ another window."
                    "\n"
                    "Most application can simply call ImGui::DockSpaceOverViewport() and be done with it.");
                ImGui::Separator();
                ImGui::TextUnformatted(
                    "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
                    "- Drag from window title bar or their tab to dock/undock." "\n"
                    "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
                    "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
                    "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)");
                ImGui::Separator();
                ImGui::TextUnformatted("More details:");
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextLinkOpenURL("Docking Wiki page", "https://github.com/ocornut/imgui/wiki/Docking");
                ImGui::BulletText("Read comments in ShowExampleAppDockSpace()");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID); node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_None);
        }

        ImGui::ShowDemoWindow();

        if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID); node && node->CentralNode) {
            ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_None);
        }

        m_ContentBrowserPanel.OnImGuiRender(globalDockspaceID, m_EditorLayerClass);

        for (const auto& workspace : m_Workspaces) {
            workspace->OnImGuiRender(globalDockspaceID);
        }
    }

    void EditorLayer::OnScenePlay() {
    }

    void EditorLayer::UI_Toolbar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        const auto& colors = ImGui::GetStyle().Colors;
        const ImVec4 hovered = colors[ImGuiCol_ButtonHovered];
        const ImVec4 active = colors[ImGuiCol_ButtonActive];

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hovered.x, hovered.y, hovered.z, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(active.x, active.y, active.z, 0.5f));

        if (ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar
                         | ImGuiWindowFlags_NoScrollWithMouse)) {
            float size = ImGui::GetWindowHeight() - 4.0f;

            Ref<Texture2D> icon = m_SceneState == SceneState::Edit
                                      ? m_IconPlay
                                      : m_IconStop;

            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - size * 2.0f);
            if (ImGui::ImageButton(icon, {size, size}, {0, 1}, {1, 0})) {
                if (m_SceneState == SceneState::Edit) {
                    //OnScenePlay();
                }
                else if (m_SceneState == SceneState::Play) {
                    //OnSceneStop();
                }
            }

            ImGui::SameLine();

            if (ImGui::ImageButton(
                m_IconPause,
                {size, size},
                {0, 1},
                {1, 0},
                m_ScenePaused ? ImVec4{0.5f, 0.5f, 0.5f, 1.0f} : ImVec4{0, 0, 0, 0}
            )) {
                m_ScenePaused = !m_ScenePaused;
            }

            ImGui::SameLine();

            if (ImGui::ImageButton(m_IconStep, {size, size}, {0, 1}, {1, 0})) {
                if (!m_ScenePaused) {
                    m_ScenePaused = true;
                }

                m_SceneStep = true;
            }

            ImGui::End();
        }
        else ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
    }

    void EditorLayer::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
            switch (e.GetKeyCode()) {
            case KeyCode::LeftControl:
            case KeyCode::RightControl:
                m_CtrlKey = true;
                break;
            case KeyCode::LeftShift:
            case KeyCode::RightShift:
                m_ShiftKey = true;
                break;
            case KeyCode::Z:
                if (m_CtrlKey && !e.IsRepeat()) {
                    m_UndoSystem.Undo();
                }
                break;
            case KeyCode::Y:
                if (m_CtrlKey && !e.IsRepeat()) {
                    m_UndoSystem.Redo();
                }
                break;
            case KeyCode::A:
                if (m_CtrlKey && m_ShiftKey && !e.IsRepeat()) {
                    m_ProjectSerializer.Serialize();
                }
            default:
                break;
            }

            return false;
        });

        dispatcher.Dispatch<KeyReleasedEvent>([this](const KeyReleasedEvent& e) {
            switch (e.GetKeyCode()) {
            case KeyCode::LeftControl:
            case KeyCode::RightControl:
                m_CtrlKey = false;
                break;
            case KeyCode::LeftShift:
            case KeyCode::RightShift:
                m_ShiftKey = false;
                break;
            default:
                break;
            }

            return false;
        });

        for (const auto& workspace : m_Workspaces) {
            workspace->OnEvent(event);
        }
    }

    void EditorLayer::OpenSceneWorkspace(const AssetID& handle) {
        Ref<Scene> scene = SceneImporter::ImportScene(handle, *Project::GetAssetManager()->GetAssetMetadata(handle));
        const Ref<SceneWorkspace> sceneWorkspace = CreateRef<SceneWorkspace>(scene, m_UndoSystem);
        m_Workspaces.push_back(sceneWorkspace);
        sceneWorkspace->Focus();
    }
}
