#pragma once
#include <Quelos/Scenes/Components.h>

namespace Quelos {
	class Window;

	class Renderer {
	public:
		static void Init(const Ref<Window>& window);
		static void Shutdown();

		static void StartFrame();
		static void StartSceneRender(const CameraComponent& Camera, const TransformComponent& CameraTransform);
		static void EndFrame();

		static void SubmitMesh();
	private:
	};
}

