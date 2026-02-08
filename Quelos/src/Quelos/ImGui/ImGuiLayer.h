#pragma once

#include "Quelos/Core/Layer.h"

namespace Quelos {
	class ImGuiLayer : public Layer {
	public:
		virtual void Begin();
		virtual void End();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void ImGuiRender() override;
	};
}
