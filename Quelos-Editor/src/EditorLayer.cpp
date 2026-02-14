#include "EditorLayer.h"
#include <imgui.h>

#include <Quelos/Scenes/Components.h>
#include <Quelos/Core/Log.h>

#include "imgui_internal.h"

#include "Quelos/Core/Base.h"

#include "Quelos/Renderer/Shader.h"
#include "Quelos/Renderer/VertexBuffer.h"
#include "Quelos/Renderer/Material.h"

#include "Quelos/Serialization/Serializer.h"

namespace Quelos {
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

    EditorLayer::EditorLayer() {
    }

    template<class... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;

    void EditorLayer::OnAttach() {
        m_DefaultScene = CreateRef<Scene>();

        const Entity camera = m_DefaultScene->CreateEntity("Camera");
        camera.Set(TransformComponent{glm::vec3(0.0f, 0.0f, -15.0f), glm::quat({0, 0, 0})});
        camera.Set(CameraComponent{SceneCamera()});

        const Entity cube = m_DefaultScene->CreateEntity("Cube");
        cube.Set(TransformComponent{glm::vec3(-2.5f, -2.5f, 0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});

        MeshComponent cubeMesh;
        cubeMesh.MeshData = CreateRef<Mesh>(cubeVertices, cubeTriList);
        cubeMesh.MaterialData = CreateRef<Material>(Shader::Create("vs_cubes.bin", "fs_cubes.bin"));
        cube.Set(cubeMesh);

        cube.Set(CubePlayer());

        const Entity cube2 = m_DefaultScene->CreateEntity("Cube2");
        cube2.Set(TransformComponent{glm::vec3(2.5f, 2.5f, 0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});
        cube2.Set(cubeMesh);
        cube2.Set(CubePlayer{-2});

        const Entity cube3 = m_DefaultScene->CreateEntity("Cube3");
        cube3.Set(TransformComponent{glm::vec3(0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});
        cube3.Set(cubeMesh);
        cube3.Set(CubePlayer{-10});

        m_DefaultScene->System<TransformComponent, CubePlayer>(
            [](const flecs::iter& it, size_t, TransformComponent& transform, CubePlayer& player) {
                player.Timer += it.delta_time();
                transform.Rotation = glm::quat({
                    player.Timer * player.Speed,
                    player.Timer * player.Speed,
                    0
                });
            }, "RotatePlayer");

        m_SceneWorkspace = CreateRef<SceneWorkspace>();
        m_SceneWorkspace->SetScene(m_DefaultScene);

        m_Workspaces.push_back(m_SceneWorkspace);

        m_EditorLayerClass.ClassId = ImHashStr("EditorLayer");
        m_EditorLayerClass.DockingAllowUnclassed = false;

        // Quel testing
        {
            using namespace Serialization;

            std::string save = R"(
[entity guid=7BB49C9FCBEBA782 name="\"Player Controller\""]
@Transform
position = (0,0,0)
rotation = (0,0,0,1)

@Waypoints
positions = {
  (1,0,0),
  (0,1,0),
  (0,0,1),
}

@Attack
attack.force.direction = (0,1,0)
attack.force.power = 15

[entity guid=267AB7E7E5700A72 name=Camera]
parent = 7BB49C9FCBEBA782

@Transform
position = (0,0,10)
rotation = (0,0,0,1)

@Camera
lens.fov = 70
)";

            std::string out("\n");
            StringQuelWriter writer(out);
            writer.SetIndent(2);
            //writer.SetFormatting(QuelFormatting::None);
            writer.Write(SectionEvent { "entity" });
            writer.Write(FieldEvent { "guid", 0 });
            writer.Write(ValueEvent { "267AB7E7E5700A72" });
            writer.Write(FieldEvent { "name", 0 });
            writer.Write(ValueEvent { "Player" });
            writer.Write(FieldEvent { "position", 0 });
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(FieldEvent { "positions", 0 });
            writer.Write(ArrayBeginEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(ArrayEndEvent{});
            writer.Write(ComponentEvent { "Transform" });
            writer.Write(FieldEvent { "position", 0 });
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(FieldEvent { "rotation", 0 });
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(ComponentEvent { "Waypoints" });
            writer.Write(FieldEvent { "positions", 0 });
            writer.Write(ArrayBeginEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(TupleBeginEvent{});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(ValueEvent {"0.0"});
            writer.Write(TupleEndEvent{});
            writer.Write(ArrayEndEvent{});

            QS_INFO("{}", out);

            QuelReader parser(out);
            for (auto&& parserEvent : parser.Parse()) {
                std::visit([](auto x) {}, parserEvent);
                std::visit(Overloaded {
                    [](const SectionEvent& event) {
                        QS_INFO("Section: {}", event.Name);
                    },
                    [](const ComponentEvent& event) {
                        QS_INFO("Component: {}", event.Name);
                    },
                    [](const FieldEvent& event) {
                        QS_INFO("Field: {}({})", event.Path, event.ID);
                    },
                    [](const ValueEvent& event) {
                        QS_INFO("Value: {}", event.Text);
                    },
                    [](const TupleBeginEvent& _) {
                        QS_INFO("Tuple Begin:");
                    },
                    [](const TupleEndEvent& _) {
                        QS_INFO("Tuple End");
                    },
                    [](const ArrayBeginEvent& _) {
                        QS_INFO("Array Start:");
                    },
                    [](const ArrayEndEvent& _) {
                        QS_INFO("Array End");
                    },
                    [](const ParseError& error) {
                        QS_INFO("Error Line {}: {}", error.Line, error.Message);
                    }
                }, parserEvent);
            }
        }
    }

    void EditorLayer::Tick(const float deltaTime) {
        for (const auto& workspace : m_Workspaces) {
            workspace->Tick(deltaTime);
        }
    }

    void EditorLayer::ImGuiRender() {
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

        ImGui::ShowDemoWindow();

        for (const auto& workspace : m_Workspaces) {
            workspace->OnImGuiRender(globalDockspaceID);
        }
    }

    void EditorLayer::OnEvent(Event& event) {
        m_SceneWorkspace->OnEvent(event);
    }
}
