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

#include "Workspaces/MaterialWorkspace/MaterialWorkspace.h"

namespace QuelosEditor {
	static void CatppuccinTheme();
	static void ImGuiDraculaStyle();
	static void IaivyTheme();
	static void SupremacyTheme();
    struct CubePlayer {
        float Speed = 2.0f;
        float Timer = 0.0f;
    };

    EditorLayer* EditorLayer::s_Instance = nullptr;
    HashMap<const char*, QS_ShaderCompiler> EditorLayer::s_ShaderCompilers;

    void EditorLayer::OnAttach() {
        s_Instance = this;

        m_ProjectSerializer = ProjectSerializer(
            Application::Get().GetApplicationPath() / "../../Quelos-Editor/SandboxProject");
        m_EditorAssetManager = RefAs<EditorAssetManager>(Project::GetAssetManager());

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

        //AssetRef<GraphicsShader> compiledShader = AssetRef<GraphicsShader>(AssetID("7d9db084-abd4-4b88-8815-ba0b4f5735b3"));

        m_EditorLayerClass.ClassId = ImHashStr("EditorLayer");
        m_EditorLayerClass.DockingAllowUnclassed = false;

        m_ContentBrowserPanel.Init();

        m_WorkspaceFactories[Scene::GetStaticType()] =
            [](UndoSystem& undoSystem, const AssetMetadata& metadata) -> Scope<Workspace> {
                return CreateScope<SceneWorkspace>(undoSystem, metadata);
            };

        m_WorkspaceFactories[Material::GetStaticType()] =
            [](UndoSystem& undoSystem, const AssetMetadata& metadata) -> Scope<Workspace> {
                return CreateScope<MaterialWorkspace>(undoSystem, metadata);
            };

        m_Themes.emplace_back("Catppuccin", CatppuccinTheme);
        m_Themes.emplace_back("Dracula", ImGuiDraculaStyle);
    	m_Themes.emplace_back("Supremacy", SupremacyTheme);
    	m_Themes.emplace_back("Iaivy", IaivyTheme);

    	ImGuiDraculaStyle();
    	m_CurrentTheme = 1;

        ImGuizmo::Create();
    }

    void EditorLayer::RegisterShaderCompiler(const char* rendererName, const QS_ShaderCompiler compiler) {
        s_ShaderCompilers[rendererName] = compiler;
    }

    void EditorLayer::Tick(const float deltaTime) {
        for (const auto& workspace : m_Workspaces | std::views::values) {
            workspace->Tick(deltaTime);
        }

        FlushOpenAssetWorkspace();
        m_EditorAssetManager->FlushReimportQueue();
    }

    void EditorLayer::ImGuiRender() {
        ImGuizmo::BeginFrame();

        static ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_NoSplit |
            ImGuiDockNodeFlags_NoResize |
            ImGuiDockNodeFlags_NoDockingOverOther;

        static ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;

        const ImGuiID globalDockspaceID = ImGui::GetID("EditorLayer_Dockspace");

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

        ImGui::Begin("##EditorDockspace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        static bool showStyleEditor = false;
        static bool showThemeSelector = false;
    	static bool showStats = false;

        // Show demo options and help
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Do Nothing")) { }

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

            if (ImGui::BeginMenu("Windows")) {
                if (ImGui::MenuItem("Style Editor")) {
                    showStyleEditor = true;
                }
            	if (ImGui::MenuItem("Theme Selector")) {
            		showThemeSelector = true;
            	}
            	if (ImGui::MenuItem("Editor Stats")) {
            		showStats = true;
            	}

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        static float statusBarHeight = ImGui::GetFrameHeight();

        const ImVec2 avail = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);

        // Dockspace area
        ImGui::BeginChild(
            "DockspaceRegion",
            ImVec2(avail.x, avail.y - statusBarHeight),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::DockSpace(globalDockspaceID, ImVec2(0, 0), dockspace_flags, &m_EditorLayerClass);

        ImGui::EndChild(); // DockspaceRegion
        ImGui::PopStyleVar(4);

        if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID)) {
            // If the only one window... no undocking
            if (node->Windows.Size == 1) {
                node->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
            }
            else {
                node->LocalFlags &= ~ImGuiDockNodeFlags_NoUndocking;
            }
        }

        // Status bar
        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0);
        ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);

        if (!m_PlayingScenes.empty()) {
            ImGui::PushStyleColor(
                ImGuiCol_ChildBg,
                ImVec4(0.1f, 0.3f, 0.1f, 1.0f)
            );
        }

        ImGui::BeginChild(
            "StatusBar",
            ImVec2(0, statusBarHeight),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::PopStyleVar(); // ImGuiStyleVar_FrameRounding

        if (!m_PlayingScenes.empty()) {
            ImGui::PopStyleColor();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Playing: ");

            ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0);
            for (const auto& sceneName : m_PlayingScenes | std::views::values) {
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(sceneName.c_str());
            }
            ImGui::PopStyleVar();
        }

        ImGui::EndChild(); // StatusBar

        ImGui::PopStyleVar(3);

        ImGui::End(); // EditorDockspace

        /* ImGui Demo */
        {
            if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID); node && node->CentralNode) {
                ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_FirstUseEver);
            }

            ImGui::SetNextWindowClass(&m_EditorLayerClass);
            ImGui::ShowDemoWindow();
        }

        /* Content Browser Panel */
        {
            if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID); node && node->CentralNode) {
                ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_FirstUseEver);
            }

            m_ContentBrowserPanel.OnImGuiRender(globalDockspaceID, m_EditorLayerClass);
        }

        if (showStyleEditor) {
            if (const ImGuiDockNode* node = ImGui::DockBuilderGetNode(globalDockspaceID); node && node->CentralNode) {
                ImGui::SetNextWindowDockID(node->CentralNode->ID, ImGuiCond_FirstUseEver);
            }
            ImGui::SetNextWindowClass(&m_EditorLayerClass);

            {
                ImGui::Begin("Style Editor", &showStyleEditor, ImGuiWindowFlags_NoCollapse);
                ImGui::ShowStyleEditor();
                ImGui::End();
            }
        }

        if (showStats) {
	        if (ImGui::Begin("Editor Stats", &showStats)) {
	        	static float timer = 0.0f;
	        	static int frameCount = 0;
	        	static float displayedFPS = 0.0f;

	        	const float deltaTime = Application::Get().GetTime()->DeltaTime();
	        	timer += deltaTime;
	        	frameCount++;

	        	if (timer >= 0.25f) {
	        		displayedFPS = static_cast<float>(frameCount) / timer;
	        		frameCount = 0;
	        		timer = 0.0f;
	        	}

	        	ImGui::Text("Frame time: %.3f ms", deltaTime * 1000.f);
	        	ImGui::Text("Frame rate: %.3f fps", displayedFPS);
	        }
        	ImGui::End();
        }

        if (showThemeSelector) {
            if (ImGui::Begin("Theme Selector", &showThemeSelector)) {
                if (ImGui::BeginCombo("Select Theme", m_Themes[m_CurrentTheme].first.c_str())) {
                    for (uint32_t i = 0; i < m_Themes.size(); i++) {
                        if (ImGui::Selectable(m_Themes[i].first.c_str(), m_CurrentTheme == i)) {
                            m_CurrentTheme = i;
                            if (m_Themes[m_CurrentTheme].second) {
                                m_Themes[m_CurrentTheme].second();
                            } else {
                                QS_WARN_TAG("EditorLayer", "Invalid theme function pointer!");
                            }
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            ImGui::End();
        }


    	std::erase_if(m_Workspaces,
					  [](const Pair<AssetID, Scope<Workspace>>& workspace) {
						  return !workspace.second->IsOpen();
					  });

        for (const auto& workspace : m_Workspaces | std::views::values) {
            workspace->OnImGuiRender(m_EditorLayerClass, globalDockspaceID);
        }
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

        for (const auto& workspace : m_Workspaces | std::views::values) {
            workspace->OnEvent(event);
        }
    }

    void EditorLayer::FlushOpenAssetWorkspace() {
        if (m_OpenAssetWorkspaceRequests.empty()) {
            return;
        }

        for (const AssetMetadata& metadata : m_OpenAssetWorkspaceRequests) {
            const auto it = m_WorkspaceFactories.find(metadata.Type);
            if (it == m_WorkspaceFactories.end()) {
                QS_CORE_WARN_TAG(
                    "No suitable workspace found for asset '{}'!",
                    metadata.FilePath
                );

                return;
            }

            if (const auto workspaceIt = m_Workspaces.find(metadata.Handle); workspaceIt != m_Workspaces.end()) {
                workspaceIt->second->Focus();
                return;
            }

            Scope<Workspace> workspace = it->second(m_UndoSystem, metadata);
            if (!workspace) {
                QS_CORE_ERROR_TAG(
                    "EditorLayer",
                    "Failed to create workspace for asset '{}'!",
                    metadata.FilePath
                );

                return;
            }

            workspace->Focus();
            m_Workspaces.emplace(metadata.Handle, std::move(workspace));
        }

        m_OpenAssetWorkspaceRequests.clear();
    }

    void EditorLayer::OpenAssetWorkspace(const AssetMetadata& metadata) {
        m_OpenAssetWorkspaceRequests.push_back(metadata);
    }

    void EditorLayer::AddPlayingScene(const SceneWorkspace* sceneWorkspace, const std::string& sceneName) {
        m_PlayingScenes.emplace(sceneWorkspace, sceneName);
    }

    void EditorLayer::RemovePlayingScene(const SceneWorkspace* sceneWorkspace) {
        m_PlayingScenes.erase(sceneWorkspace);
    }

	static void CatppuccinTheme() {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// Catppuccin Mocha Palette
		// --------------------------------------------------------
		constexpr ImVec4 base = ImVec4(0.117f, 0.117f, 0.172f, 1.0f); // #1e1e2e
		constexpr ImVec4 mantle = ImVec4(0.109f, 0.109f, 0.156f, 1.0f); // #181825
		constexpr ImVec4 surface0 = ImVec4(0.200f, 0.207f, 0.286f, 1.0f); // #313244
		constexpr ImVec4 surface1 = ImVec4(0.247f, 0.254f, 0.337f, 1.0f); // #3f4056
		constexpr ImVec4 surface2 = ImVec4(0.290f, 0.301f, 0.388f, 1.0f); // #4a4d63
		constexpr ImVec4 overlay0 = ImVec4(0.396f, 0.403f, 0.486f, 1.0f); // #65677c
		constexpr ImVec4 overlay2 = ImVec4(0.576f, 0.584f, 0.654f, 1.0f); // #9399b2
		constexpr ImVec4 text = ImVec4(0.803f, 0.815f, 0.878f, 1.0f); // #cdd6f4
		constexpr ImVec4 subtext0 = ImVec4(0.639f, 0.658f, 0.764f, 1.0f); // #a3a8c3
		constexpr ImVec4 mauve = ImVec4(0.796f, 0.698f, 0.972f, 1.0f); // #cba6f7
		constexpr ImVec4 peach = ImVec4(0.980f, 0.709f, 0.572f, 1.0f); // #fab387
		constexpr ImVec4 yellow = ImVec4(0.980f, 0.913f, 0.596f, 1.0f); // #f9e2af
		constexpr ImVec4 green = ImVec4(0.650f, 0.890f, 0.631f, 1.0f); // #a6e3a1
		constexpr ImVec4 teal = ImVec4(0.580f, 0.886f, 0.819f, 1.0f); // #94e2d5
		constexpr ImVec4 sapphire = ImVec4(0.458f, 0.784f, 0.878f, 1.0f); // #74c7ec
		constexpr ImVec4 blue = ImVec4(0.533f, 0.698f, 0.976f, 1.0f); // #89b4fa
		constexpr ImVec4 lavender = ImVec4(0.709f, 0.764f, 0.980f, 1.0f); // #b4befe

		// Main window and backgrounds
		colors[ImGuiCol_WindowBg] = base;
		colors[ImGuiCol_ChildBg] = base;
		colors[ImGuiCol_PopupBg] = surface0;
		colors[ImGuiCol_Border] = surface1;
		colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_FrameBg] = surface0;
		colors[ImGuiCol_FrameBgHovered] = surface1;
		colors[ImGuiCol_FrameBgActive] = surface2;
		colors[ImGuiCol_TitleBg] = mantle;
		colors[ImGuiCol_TitleBgActive] = surface0;
		colors[ImGuiCol_TitleBgCollapsed] = mantle;
		colors[ImGuiCol_MenuBarBg] = mantle;
		colors[ImGuiCol_ScrollbarBg] = surface0;
		colors[ImGuiCol_ScrollbarGrab] = surface2;
		colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
		colors[ImGuiCol_ScrollbarGrabActive] = overlay2;
		colors[ImGuiCol_CheckMark] = green;
		colors[ImGuiCol_SliderGrab] = sapphire;
		colors[ImGuiCol_SliderGrabActive] = blue;
		colors[ImGuiCol_Button] = surface0;
		colors[ImGuiCol_ButtonHovered] = surface1;
		colors[ImGuiCol_ButtonActive] = surface2;
		colors[ImGuiCol_Header] = surface0;
		colors[ImGuiCol_HeaderHovered] = surface1;
		colors[ImGuiCol_HeaderActive] = surface2;
		colors[ImGuiCol_Separator] = surface1;
		colors[ImGuiCol_SeparatorHovered] = mauve;
		colors[ImGuiCol_SeparatorActive] = mauve;
		colors[ImGuiCol_ResizeGrip] = surface2;
		colors[ImGuiCol_ResizeGripHovered] = mauve;
		colors[ImGuiCol_ResizeGripActive] = mauve;
		colors[ImGuiCol_Tab] = surface0;
		colors[ImGuiCol_TabHovered] = surface2;
		colors[ImGuiCol_TabActive] = surface1;
		colors[ImGuiCol_TabUnfocused] = surface0;
		colors[ImGuiCol_TabUnfocusedActive] = surface1;
		colors[ImGuiCol_DockingPreview] = sapphire;
		colors[ImGuiCol_DockingEmptyBg] = base;
		colors[ImGuiCol_PlotLines] = blue;
		colors[ImGuiCol_PlotLinesHovered] = peach;
		colors[ImGuiCol_PlotHistogram] = teal;
		colors[ImGuiCol_PlotHistogramHovered] = green;
		colors[ImGuiCol_TableHeaderBg] = surface0;
		colors[ImGuiCol_TableBorderStrong] = surface1;
		colors[ImGuiCol_TableBorderLight] = surface0;
		colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = surface2;
		colors[ImGuiCol_DragDropTarget] = yellow;
		colors[ImGuiCol_NavHighlight] = lavender;
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
		colors[ImGuiCol_Text] = text;
		colors[ImGuiCol_TextDisabled] = subtext0;

		// Rounded corners
		style.WindowRounding = 6.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 9.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;

		// Padding and spacing
		style.WindowPadding = ImVec2(8.0f, 8.0f);
		style.FramePadding = ImVec2(5.0f, 3.0f);
		style.ItemSpacing = ImVec2(8.0f, 4.0f);
		style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
		style.IndentSpacing = 21.0f;
		style.ScrollbarSize = 14.0f;
		style.GrabMinSize = 10.0f;

		// Borders
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;
	}

	static void ImGuiDraculaStyle() {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// --- 1. Sizing and Spacing (Clean & Balanced) ---
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ScrollbarSize = 14.0f;
		style.GrabMinSize = 12.0f;

		// --- 2. Borders & Rounding ---
		style.WindowRounding = 6.0f;
		style.FrameRounding = 4.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 12.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;

		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;

		// --- 3. The Dracula Color Palette ---
		// Background: #282a36 | Selection: #44475a | Foreground: #f8f8f2
		// Comment: #6272a4    | Cyan: #8be9fd      | Green: #50fa7b
		// Orange: #ffb86c     | Pink: #ff79c6      | Purple: #bd93f9
		// Red: #ff5555        | Yellow: #f1fa8c

		// Text
		colors[ImGuiCol_Text] = ImVec4(0.97f, 0.97f, 0.95f, 1.00f); // #f8f8f2
		colors[ImGuiCol_TextDisabled] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f); // #6272a4

		// Backgrounds
		colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f); // #282a36
		colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.21f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.21f, 0.96f);

		// Borders
		colors[ImGuiCol_Border] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f); // #44475a
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		// Frames (Inputs, etc.)
		colors[ImGuiCol_FrameBg] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f); // #44475a
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f); // #6272a4
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.48f, 0.55f, 0.74f, 1.00f);

		// Title Bars
		colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f); // Darker
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);

		// Menus
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.55f, 0.74f, 1.00f);

		// Interactables
		colors[ImGuiCol_CheckMark] = ImVec4(0.31f, 0.98f, 0.48f, 1.00f); // #50fa7b (Green)
		colors[ImGuiCol_SliderGrab] = ImVec4(0.74f, 0.58f, 0.98f, 1.00f); // #bd93f9 (Purple)
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.84f, 0.68f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.47f, 0.78f, 1.00f); // #ff79c6 (Pink)
		colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.37f, 0.62f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.55f, 0.74f, 1.00f);

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);

		// Misc
		colors[ImGuiCol_PlotLines] = ImVec4(0.55f, 0.91f, 0.99f, 1.00f); // #8be9fd (Cyan)
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);

#ifdef IMGUI_HAS_DOCK
		colors[ImGuiCol_DockingPreview] = ImVec4(0.74f, 0.58f, 0.98f, 0.50f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
#endif
	}

	static void IaivyTheme() {
		auto& style{ImGui::GetStyle()};
		// Borders
		style.WindowBorderSize = 3.0f;

		// Rounding
		style.FrameRounding = 3.0f;
		style.PopupRounding = 3.0f;
		style.ScrollbarRounding = 3.0f;
		style.GrabRounding = 3.0f;

		// Docking
		style.DockingSeparatorSize = 3.0f;

		constexpr auto toRGBA = [](uint32_t argb) constexpr {
			ImVec4 color{};
			color.x = ((argb >> 16) & 0xFF) / 255.0f;
			color.y = ((argb >> 8) & 0xFF) / 255.0f;
			color.z = (argb & 0xFF) / 255.0f;
			color.w = ((argb >> 24) & 0xFF) / 255.0f;
			return color;
		};

		constexpr auto lerp = [](const ImVec4& a, const ImVec4& b, float t) constexpr {
			return ImVec4{
				std::lerp(a.x, b.y, t),
				std::lerp(a.y, b.y, t),
				std::lerp(a.z, b.z, t),
				std::lerp(a.w, b.w, t)
			};
		};

		auto colors{style.Colors};
		colors[ImGuiCol_Text] = toRGBA(0xFFABB2BF);
		colors[ImGuiCol_TextDisabled] = toRGBA(0xFF565656);
		colors[ImGuiCol_WindowBg] = toRGBA(0xFF282C34);
		colors[ImGuiCol_ChildBg] = toRGBA(0xFF21252B);
		colors[ImGuiCol_PopupBg] = toRGBA(0xFF2E323A);
		colors[ImGuiCol_Border] = toRGBA(0xFF2E323A);
		colors[ImGuiCol_BorderShadow] = toRGBA(0x00000000);
		colors[ImGuiCol_FrameBg] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_FrameBgHovered] = toRGBA(0xFF484C52);
		colors[ImGuiCol_FrameBgActive] = toRGBA(0xFF54575D);
		colors[ImGuiCol_TitleBg] = colors[ImGuiCol_WindowBg];
		colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_TitleBgCollapsed] = toRGBA(0x8221252B);
		colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_ScrollbarBg] = colors[ImGuiCol_PopupBg];
		colors[ImGuiCol_ScrollbarGrab] = toRGBA(0xFF3E4249);
		colors[ImGuiCol_ScrollbarGrabHovered] = toRGBA(0xFF484C52);
		colors[ImGuiCol_ScrollbarGrabActive] = toRGBA(0xFF54575D);
		colors[ImGuiCol_CheckMark] = colors[ImGuiCol_Text];
		colors[ImGuiCol_SliderGrab] = toRGBA(0xFF353941);
		colors[ImGuiCol_SliderGrabActive] = toRGBA(0xFF7A7A7A);
		colors[ImGuiCol_Button] = colors[ImGuiCol_SliderGrab];
		colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_ScrollbarGrabActive];
		colors[ImGuiCol_Header] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_HeaderHovered] = toRGBA(0xFF353941);
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_Separator] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_SeparatorHovered] = toRGBA(0xFF3E4452);
		colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_SeparatorHovered];
		colors[ImGuiCol_ResizeGrip] = colors[ImGuiCol_Separator];
		colors[ImGuiCol_ResizeGripHovered] = colors[ImGuiCol_SeparatorHovered];
		colors[ImGuiCol_ResizeGripActive] = colors[ImGuiCol_SeparatorActive];
		colors[ImGuiCol_InputTextCursor] = toRGBA(0xFF528BFF);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_Tab] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_TabSelected] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
		colors[ImGuiCol_TabDimmed] = lerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabDimmedSelected] = lerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4{0.50f, 0.50f, 0.50f, 0.00f};
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
		colors[ImGuiCol_PlotLines] = ImVec4{0.61f, 0.61f, 0.61f, 1.00f};
		colors[ImGuiCol_PlotLinesHovered] = ImVec4{1.00f, 0.43f, 0.35f, 1.00f};
		colors[ImGuiCol_PlotHistogram] = ImVec4{0.90f, 0.70f, 0.00f, 1.00f};
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4{1.00f, 0.60f, 0.00f, 1.00f};
		colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_SliderGrab];
		colors[ImGuiCol_TableBorderLight] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_TableRowBg] = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
		colors[ImGuiCol_TableRowBgAlt] = ImVec4{1.00f, 1.00f, 1.00f, 0.06f};
		colors[ImGuiCol_TextLink] = toRGBA(0xFF3F94CE);
		colors[ImGuiCol_TextSelectedBg] = toRGBA(0xFF243140);
		colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Text];
		colors[ImGuiCol_DragDropTarget] = colors[ImGuiCol_Text];
		colors[ImGuiCol_NavCursor] = colors[ImGuiCol_TextLink];
		colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_Text];
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.80f, 0.80f, 0.80f, 0.20f};
		colors[ImGuiCol_ModalWindowDimBg] = toRGBA(0xC821252B);
	}

	static void SupremacyTheme() {
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		// Base colors for a pleasant and modern dark theme with dark accents
		colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f); // Light grey text for readability
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.54f, 1.00f); // Subtle grey for disabled text
		colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Dark background with a hint of blue
		colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Slightly lighter for child elements
		colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f); // Popup background
		colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.30f, 0.60f); // Soft border color
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border shadow
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Frame background
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Frame hover effect
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f); // Active frame background
		colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Title background
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Active title background
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Collapsed title background
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f); // Menu bar background
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Scrollbar background
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f); // Dark accent for scrollbar grab
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f); // Scrollbar grab hover
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f); // Scrollbar grab active
		colors[ImGuiCol_CheckMark] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Dark blue checkmark
		colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Dark blue slider grab
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Active slider grab
		colors[ImGuiCol_Button] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Dark blue button
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Button hover effect
		colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active button
		colors[ImGuiCol_Header] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Header color similar to button
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Header hover effect
		colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active header
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Separator color
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for separator
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Active separator
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Resize grip
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for resize grip
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.54f, 0.64f, 1.00f); // Active resize grip
		colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Inactive tab
		colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Hover effect for tab
		colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Active tab color
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Unfocused tab
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Active but unfocused tab
		colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Plot lines
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for plot lines
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Histogram color
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for histogram
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table header background
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Strong border for tables
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.25f, 0.26f, 1.00f); // Light border for tables
		colors[ImGuiCol_TableRowBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table row background
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Alternate row background
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.34f, 0.44f, 0.35f); // Selected text background
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.56f, 0.66f, 0.90f); // Drag and drop target
		colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Navigation highlight
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f); // Dim background for windowing
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f); // Dim background for modal windows

		// Style adjustments
		style->WindowPadding = ImVec2(8.00f, 8.00f);
		style->FramePadding = ImVec2(5.00f, 2.00f);
		style->CellPadding = ImVec2(6.00f, 6.00f);
		style->ItemSpacing = ImVec2(6.00f, 6.00f);
		style->ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style->TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style->IndentSpacing = 25;
		style->ScrollbarSize = 11;
		style->GrabMinSize = 10;
		style->WindowBorderSize = 1;
		style->ChildBorderSize = 1;
		style->PopupBorderSize = 1;
		style->FrameBorderSize = 1;
		style->TabBorderSize = 1;
		style->WindowRounding = 7;
		style->ChildRounding = 4;
		style->FrameRounding = 3;
		style->PopupRounding = 4;
		style->ScrollbarRounding = 9;
		style->GrabRounding = 3;
		style->LogSliderDeadzone = 4;
		style->TabRounding = 4;
	}
}
