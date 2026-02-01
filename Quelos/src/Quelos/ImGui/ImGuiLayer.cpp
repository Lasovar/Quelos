#include <qspch.h>

#include "ImGuiLayer.h"
#include <imgui.h>
#ifdef QUELOS_USE_SDL
#include "imgui_impl_sdl3.h"
#elif QUELOS_USE_GLFW
#include "imgui_impl_glfw.h"
#endif

#include <Quelos/Core/Application.h>

#include "ImGuiState.h"

namespace Quelos {
	static void CatppuccinTheme();
	static void IaivyTheme();
	static void SupremacyTheme();

	static Ref<ImGuiState> s_ImGuiState;

	static std::function<void()> s_ImGuiWindowImpl_NewFrameFn;

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

		s_ImGuiState->Init(17);

		const Application& app = Application::Get();
		const Ref<Window> window = app.GetWindow();

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
		}

		CatppuccinTheme();
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
		}

		s_ImGuiState->Destroy();
	}

	void ImGuiLayer::ImGuiRender() {
		static std::vector<std::string> themes = { "Catppuccin", "Iaivy", "Supremacy" };
		static int currentTheme = 0;
		if (ImGui::Begin("Theme Selector")) {
			if (ImGui::BeginCombo("Select Theme", themes[currentTheme].c_str())) {
				for (int i = 0; i < themes.size(); i++) {
					if (ImGui::Selectable(themes[i].c_str(), currentTheme == i)) {
						currentTheme = i;
						switch (currentTheme) {
						case 0:
							CatppuccinTheme();
							break;
						case 1:
							IaivyTheme();
							break;
						case 2:
							SupremacyTheme();
							break;
						default:
							break;
						}
					}
				}

				ImGui::EndCombo();
			}

			ImGui::End();
		} else ImGui::End();
	}

	static void CatppuccinTheme() {
	    ImGuiStyle& style = ImGui::GetStyle();
	    ImVec4* colors = style.Colors;

	    // Catppuccin Mocha Palette
	    // --------------------------------------------------------
	    constexpr ImVec4 base       = ImVec4(0.117f, 0.117f, 0.172f, 1.0f); // #1e1e2e
	    constexpr ImVec4 mantle     = ImVec4(0.109f, 0.109f, 0.156f, 1.0f); // #181825
	    constexpr ImVec4 surface0   = ImVec4(0.200f, 0.207f, 0.286f, 1.0f); // #313244
	    constexpr ImVec4 surface1   = ImVec4(0.247f, 0.254f, 0.337f, 1.0f); // #3f4056
	    constexpr ImVec4 surface2   = ImVec4(0.290f, 0.301f, 0.388f, 1.0f); // #4a4d63
	    constexpr ImVec4 overlay0   = ImVec4(0.396f, 0.403f, 0.486f, 1.0f); // #65677c
	    constexpr ImVec4 overlay2   = ImVec4(0.576f, 0.584f, 0.654f, 1.0f); // #9399b2
	    constexpr ImVec4 text       = ImVec4(0.803f, 0.815f, 0.878f, 1.0f); // #cdd6f4
	    constexpr ImVec4 subtext0   = ImVec4(0.639f, 0.658f, 0.764f, 1.0f); // #a3a8c3
	    constexpr ImVec4 mauve      = ImVec4(0.796f, 0.698f, 0.972f, 1.0f); // #cba6f7
	    constexpr ImVec4 peach      = ImVec4(0.980f, 0.709f, 0.572f, 1.0f); // #fab387
	    constexpr ImVec4 yellow     = ImVec4(0.980f, 0.913f, 0.596f, 1.0f); // #f9e2af
	    constexpr ImVec4 green      = ImVec4(0.650f, 0.890f, 0.631f, 1.0f); // #a6e3a1
	    constexpr ImVec4 teal       = ImVec4(0.580f, 0.886f, 0.819f, 1.0f); // #94e2d5
	    constexpr ImVec4 sapphire   = ImVec4(0.458f, 0.784f, 0.878f, 1.0f); // #74c7ec
	    constexpr ImVec4 blue       = ImVec4(0.533f, 0.698f, 0.976f, 1.0f); // #89b4fa
	    constexpr ImVec4 lavender   = ImVec4(0.709f, 0.764f, 0.980f, 1.0f); // #b4befe

	    // Main window and backgrounds
	    colors[ImGuiCol_WindowBg]             = base;
	    colors[ImGuiCol_ChildBg]              = base;
	    colors[ImGuiCol_PopupBg]              = surface0;
	    colors[ImGuiCol_Border]               = surface1;
	    colors[ImGuiCol_BorderShadow]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	    colors[ImGuiCol_FrameBg]              = surface0;
	    colors[ImGuiCol_FrameBgHovered]       = surface1;
	    colors[ImGuiCol_FrameBgActive]        = surface2;
	    colors[ImGuiCol_TitleBg]              = mantle;
	    colors[ImGuiCol_TitleBgActive]        = surface0;
	    colors[ImGuiCol_TitleBgCollapsed]     = mantle;
	    colors[ImGuiCol_MenuBarBg]            = mantle;
	    colors[ImGuiCol_ScrollbarBg]          = surface0;
	    colors[ImGuiCol_ScrollbarGrab]        = surface2;
	    colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
	    colors[ImGuiCol_ScrollbarGrabActive]  = overlay2;
	    colors[ImGuiCol_CheckMark]            = green;
	    colors[ImGuiCol_SliderGrab]           = sapphire;
	    colors[ImGuiCol_SliderGrabActive]     = blue;
	    colors[ImGuiCol_Button]               = surface0;
	    colors[ImGuiCol_ButtonHovered]        = surface1;
	    colors[ImGuiCol_ButtonActive]         = surface2;
	    colors[ImGuiCol_Header]               = surface0;
	    colors[ImGuiCol_HeaderHovered]        = surface1;
	    colors[ImGuiCol_HeaderActive]         = surface2;
	    colors[ImGuiCol_Separator]            = surface1;
	    colors[ImGuiCol_SeparatorHovered]     = mauve;
	    colors[ImGuiCol_SeparatorActive]      = mauve;
	    colors[ImGuiCol_ResizeGrip]           = surface2;
	    colors[ImGuiCol_ResizeGripHovered]    = mauve;
	    colors[ImGuiCol_ResizeGripActive]     = mauve;
	    colors[ImGuiCol_Tab]                  = surface0;
	    colors[ImGuiCol_TabHovered]           = surface2;
	    colors[ImGuiCol_TabActive]            = surface1;
	    colors[ImGuiCol_TabUnfocused]         = surface0;
	    colors[ImGuiCol_TabUnfocusedActive]   = surface1;
	    colors[ImGuiCol_DockingPreview]       = sapphire;
	    colors[ImGuiCol_DockingEmptyBg]       = base;
	    colors[ImGuiCol_PlotLines]            = blue;
	    colors[ImGuiCol_PlotLinesHovered]     = peach;
	    colors[ImGuiCol_PlotHistogram]        = teal;
	    colors[ImGuiCol_PlotHistogramHovered] = green;
	    colors[ImGuiCol_TableHeaderBg]        = surface0;
	    colors[ImGuiCol_TableBorderStrong]    = surface1;
	    colors[ImGuiCol_TableBorderLight]     = surface0;
	    colors[ImGuiCol_TableRowBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	    colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	    colors[ImGuiCol_TextSelectedBg]       = surface2;
	    colors[ImGuiCol_DragDropTarget]       = yellow;
	    colors[ImGuiCol_NavHighlight]         = lavender;
	    colors[ImGuiCol_NavWindowingHighlight]= ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	    colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	    colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
	    colors[ImGuiCol_Text]                 = text;
	    colors[ImGuiCol_TextDisabled]         = subtext0;

	    // Rounded corners
	    style.WindowRounding    = 6.0f;
	    style.ChildRounding     = 6.0f;
	    style.FrameRounding     = 4.0f;
	    style.PopupRounding     = 4.0f;
	    style.ScrollbarRounding = 9.0f;
	    style.GrabRounding      = 4.0f;
	    style.TabRounding       = 4.0f;

	    // Padding and spacing
	    style.WindowPadding     = ImVec2(8.0f, 8.0f);
	    style.FramePadding      = ImVec2(5.0f, 3.0f);
	    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
	    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
	    style.IndentSpacing     = 21.0f;
	    style.ScrollbarSize     = 14.0f;
	    style.GrabMinSize       = 10.0f;

	    // Borders
	    style.WindowBorderSize  = 1.0f;
	    style.ChildBorderSize   = 1.0f;
	    style.PopupBorderSize   = 1.0f;
	    style.FrameBorderSize   = 0.0f;
	    style.TabBorderSize     = 0.0f;
	}

	static void IaivyTheme() {
		auto& style{ ImGui::GetStyle() };
		// Borders
		style.WindowBorderSize = 3.0f;

		// Rounding
		style.FrameRounding = 3.0f;
		style.PopupRounding = 3.0f;
		style.ScrollbarRounding = 3.0f;
		style.GrabRounding = 3.0f;

		// Docking
		style.DockingSeparatorSize = 3.0f;

		constexpr auto toRGBA = [](uint32_t argb) constexpr
		{
		    ImVec4 color{};
		    color.x = ((argb >> 16) & 0xFF) / 255.0f;
		    color.y = ((argb >> 8) & 0xFF) / 255.0f;
		    color.z = (argb & 0xFF) / 255.0f;
		    color.w = ((argb >> 24) & 0xFF) / 255.0f;
		    return color;
		};

		constexpr auto lerp = [](const ImVec4& a, const ImVec4& b, float t) constexpr
		{
		    return ImVec4{
		        std::lerp(a.x, b.y, t),
		        std::lerp(a.y, b.y, t),
		        std::lerp(a.z, b.z, t),
		        std::lerp(a.w, b.w, t)
		    };
		};

		auto colors{ style.Colors };
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
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4{ 0.50f, 0.50f, 0.50f, 0.00f };
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
		colors[ImGuiCol_PlotLines] = ImVec4{ 0.61f, 0.61f, 0.61f, 1.00f };
		colors[ImGuiCol_PlotLinesHovered] = ImVec4{ 1.00f, 0.43f, 0.35f, 1.00f };
		colors[ImGuiCol_PlotHistogram] = ImVec4{ 0.90f, 0.70f, 0.00f, 1.00f };
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4{ 1.00f, 0.60f, 0.00f, 1.00f };
		colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_ChildBg];
		colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_SliderGrab];
		colors[ImGuiCol_TableBorderLight] = colors[ImGuiCol_FrameBgActive];
		colors[ImGuiCol_TableRowBg] = ImVec4{ 0.00f, 0.00f, 0.00f, 0.00f };
		colors[ImGuiCol_TableRowBgAlt] = ImVec4{ 1.00f, 1.00f, 1.00f, 0.06f };
		colors[ImGuiCol_TextLink] = toRGBA(0xFF3F94CE);
		colors[ImGuiCol_TextSelectedBg] = toRGBA(0xFF243140);
		colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Text];
		colors[ImGuiCol_DragDropTarget] = colors[ImGuiCol_Text];
		colors[ImGuiCol_NavCursor] = colors[ImGuiCol_TextLink];
		colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_Text];
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4{ 0.80f, 0.80f, 0.80f, 0.20f };
		colors[ImGuiCol_ModalWindowDimBg] = toRGBA(0xC821252B);
	}

	static void SupremacyTheme() {
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		// Base colors for a pleasant and modern dark theme with dark accents
		colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);                  // Light grey text for readability
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);          // Subtle grey for disabled text
		colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);              // Dark background with a hint of blue
		colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);               // Slightly lighter for child elements
		colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);               // Popup background
		colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);                // Soft border color
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);          // No border shadow
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);               // Frame background
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);        // Frame hover effect
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Active frame background
		colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);               // Title background
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);         // Active title background
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);      // Collapsed title background
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);             // Menu bar background
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);           // Scrollbar background
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Dark accent for scrollbar grab
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);  // Scrollbar grab hover
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);   // Scrollbar grab active
		colors[ImGuiCol_CheckMark] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Dark blue checkmark
		colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Dark blue slider grab
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);      // Active slider grab
		colors[ImGuiCol_Button] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Dark blue button
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Button hover effect
		colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active button
		colors[ImGuiCol_Header] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Header color similar to button
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Header hover effect
		colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active header
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);             // Separator color
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for separator
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);       // Active separator
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Resize grip
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);     // Hover effect for resize grip
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);      // Active resize grip
		colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);                   // Inactive tab
		colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);            // Hover effect for tab
		colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);             // Active tab color
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);          // Unfocused tab
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);    // Active but unfocused tab
		colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Plot lines
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for plot lines
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);         // Histogram color
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);  // Hover effect for histogram
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);         // Table header background
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);     // Strong border for tables
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);      // Light border for tables
		colors[ImGuiCol_TableRowBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);            // Table row background
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);         // Alternate row background
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.34f, 0.44f, 0.35f);        // Selected text background
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);        // Drag and drop target
		colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);          // Navigation highlight
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);     // Dim background for windowing
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);      // Dim background for modal windows

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
