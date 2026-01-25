#pragma once
#include <Quelos/Scenes/Components.h>

#include "Quelos/Core/Event.h"

namespace Quelos {
	class Window;

	class Renderer {
	public:
		static void Init(const Ref<Window>& window);
		static void Shutdown();

		static void OnEvent(Event& event);

		static void StartFrame();
		static void StartSceneRender(const CameraComponent& Camera, const TransformComponent& CameraTransform);
		static void EndFrame();

		static void SubmitMesh();
	private:
	};
}

