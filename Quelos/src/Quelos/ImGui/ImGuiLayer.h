#pragma once

#include "Quelos/Core/Layer.h"
#include "imgui.h"
#include "imgui_user.h"

namespace Quelos {
	class QS_API ImGuiLayer : public Layer {
	public:
		explicit ImGuiLayer(const std::string& name = "ImGui Layer")
			: Layer(name) {}

		virtual void Begin();
		virtual void End();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void ImGuiRender() override;

	private:
        ImFont* m_Font[ImGui::Font::Count]{};
	};
}
