#pragma once

#include "Quelos/Core/Layer.h"
#include "SDL3/SDL_events.h"

namespace Quelos {
	class ImGuiLayer : public Layer {
	public:
		virtual void Begin();
		virtual void End();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void ImGuiRender() override;
	public:
		// TODO: TEMP
		static void OnSDL_Event(const SDL_Event* event);
	};
}
