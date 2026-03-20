#pragma once

#include "Quelos/Core/Layer.h"

namespace Quelos {
	class ImGuiLayer : public Layer {
	public:
		explicit ImGuiLayer(const std::string& name = "ImGui Layer")
			: Layer(name) {}

		virtual void Begin();
		virtual void End();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void ImGuiRender() override;
	};
}
