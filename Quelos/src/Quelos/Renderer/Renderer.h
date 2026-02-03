#pragma once
#include <Quelos/Scenes/Components.h>

#include "FrameBuffer.h"
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
		static void StartSceneRender(uint32_t viewId, const Ref<FrameBuffer>& frameBuffer, const CameraComponent& Camera, const TransformComponent& CameraTransform);
		static void EndFrame();

		static void SubmitMesh(uint32_t viewId, const MeshComponent& mesh, const TransformComponent& transform);
	};
}

