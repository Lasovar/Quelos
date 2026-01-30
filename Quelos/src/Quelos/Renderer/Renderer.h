#pragma once
#include <Quelos/Scenes/Components.h>

#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/RendererAPI.h"

namespace Quelos {
	class Window;

	class Renderer {
	public:
		static bool IsInitialized();
		static void Init(const Ref<Window>& window, RendererAPI api);
		static void Shutdown();

		static void OnEvent(Event& event);

		static void StartFrame();
		static void StartSceneRender(const CameraComponent& Camera, const TransformComponent& CameraTransform);
		static void EndFrame();

		static void SubmitMesh(const MeshComponent& mesh, const TransformComponent& transform);
	};
}

