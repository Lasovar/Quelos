#include <qspch.h>

#include "ImGuiLayer.h"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_bgfx.h"
#include "bgfx/bgfx.h"
#include <Quelos/Core/Application.h>

namespace Quelos {

	void ImGuiLayer::Begin() {
		ImGui_ImplGlfw_NewFrame();
		ImGui_Implbgfx_NewFrame();

		ImGui::NewFrame();
	}

	void ImGuiLayer::End() {
		ImGui::Render();
		ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
	}

	void ImGuiLayer::OnAttach() {
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		ImGui::StyleColorsDark();

		Application& app = Application::Get();
		Ref<Window> window = app.GetWindow();

		ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(window->GetWindowHandle()), true);
		ImGui_Implbgfx_Init(255);
	}

	void ImGuiLayer::OnDetach() {
		ImGui_Implbgfx_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();
	}

	void ImGuiLayer::ImGuiRender() {
	}

}

