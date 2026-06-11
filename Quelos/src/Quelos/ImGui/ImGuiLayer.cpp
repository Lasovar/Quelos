#include "ImGuiLayer.h"

#include <qspch.h>

#define IMGUI_ENABLE_FREETYPE
#include "Quelos/ImGui/imgui_freetype.h"

#include "icons_font_awesome.h"
#include "icons_font_awesome.ttf.h"
#include "icons_kenney.h"
#include "icons_kenney.ttf.h"
#include "roboto_regular.ttf.h"
#include "robotomono_regular.ttf.h"

#ifdef QUELOS_USE_SDL
#include "Quelos/Platform/Window/imgui_impl_sdl3.h"
#elif QUELOS_USE_GLFW
#include "Quelos/Platform/Window/imgui_impl_glfw.h"
#endif

#include <Quelos/Core/Application.h>

#include "ImGuiState.h"

namespace Quelos {
	static Ref<ImGuiState> s_ImGuiState;

	static std::function<void()> s_ImGuiWindowImpl_NewFrameFn;

	struct FontRangeMerge {
		const void* Data;
		size_t Size;
		ImWchar Ranges[3];
	};

	static FontRangeMerge s_FontRangeMerge[] = {
		{s_iconsKenneyTtf, sizeof(s_iconsKenneyTtf), {ICON_MIN_KI, ICON_MAX_KI, 0}},
		{s_iconsFontAwesomeTtf, sizeof(s_iconsFontAwesomeTtf), {ICON_MIN_FA, ICON_MAX_FA, 0}},
	};

	void ImGuiLayer::Begin() {
		s_ImGuiWindowImpl_NewFrameFn();
		s_ImGuiState->BeginFrame(255);
	}

	void ImGuiLayer::End() {
		s_ImGuiState->EndFrame();
	}

	void ImGuiLayer::OnAttach() {
		if (!s_ImGuiState) {
			s_ImGuiState = ImGuiState::Create();
		}

		s_ImGuiState->Init();
		Ref<Window> window = Application::Get().GetWindow();

		const float scaleFactor = window->GetDisplayScaling();
		const float fontSize = 17 * scaleFactor;

		s_ImGuiState->Init();

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.ConfigDragClickToInputText = true;
		io.FontGlobalScale = 1.0f / scaleFactor;
		io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);

		ImGui::GetStyle().ScaleAllSizes(scaleFactor);

		{
			ImFontConfig config;
			config.FontDataOwnedByAtlas = false;
			config.MergeMode = false;

			io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

			const ImWchar* ranges = io.Fonts->GetGlyphRangesDefault();
			m_Font[ImGui::Font::Regular] = io.Fonts->AddFontFromMemoryTTF(
				(void*)s_robotoRegularTtf,
				sizeof(s_robotoRegularTtf),
				fontSize,
				&config,
				ranges
			);

			m_Font[ImGui::Font::Mono] = io.Fonts->AddFontFromMemoryTTF(
				(void*)s_robotoMonoRegularTtf,
				sizeof(s_robotoMonoRegularTtf),
				fontSize - 3.0f,
				&config,
				ranges
			);

			config.MergeMode = true;
			config.DstFont = m_Font[ImGui::Font::Regular];

			for (const auto& frm : s_FontRangeMerge) {
				io.Fonts->AddFontFromMemoryTTF(
					const_cast<void*>(frm.Data),
					static_cast<int>(frm.Size),
					fontSize - 3.0f,
					&config,
					frm.Ranges
				);
			}
		}

		switch (window->GetWindowBacked()) {
		case WindowingBackend::None:
			break;
		case WindowingBackend::SDL:
#ifdef QUELOS_USE_SDL
			ImGui_ImplSDL3_InitForOther(static_cast<SDL_Window*>(window->GetWindowHandle()));
			s_ImGuiWindowImpl_NewFrameFn = ImGui_ImplSDL3_NewFrame;
			break;
#elif QUELOS_USE_GLFW
		case WindowingBackend::GLFW:
			ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(window->GetWindowHandle()), true);
			s_ImGuiWindowImpl_NewFrameFn = ImGui_ImplGlfw_NewFrame;
			break;
#endif
			default:
			QS_ASSERT(false, "Unknown windowing backend");
			break;
		}

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 2.0f;
		style.FramePadding = ImVec2(6, 2);
		style.ItemSpacing = ImVec2(8, 4);
		style.ColorButtonPosition = ImGuiDir_Left;
	}

	void ImGuiLayer::OnDetach() {
		switch (Application::Get().GetWindow()->GetWindowBacked()) {
		case WindowingBackend::None:
			break;
		case WindowingBackend::SDL:
#ifdef QUELOS_USE_SDL
			ImGui_ImplSDL3_Shutdown();
			break;
#elif QUELOS_USE_GLFW
		case WindowingBackend::GLFW:
			ImGui_ImplGlfw_Shutdown();
			break;
#endif
		default:
			QS_ASSERT(false, "Unknown windowing backend");
			break;
		}

		s_ImGuiState->Destroy();
	}

	void ImGuiLayer::ImGuiRender() { }

}
