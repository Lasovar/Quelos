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
		static void StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const TransformComponent& transform, const glm::mat4& projection);
		static void StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const glm::mat4& view, const glm::mat4& projection);
		static void EndFrame();

		static void SubmitMesh(uint32_t viewID, const MeshComponent& mesh, const TransformComponent& transform);

		// Renderer Context API
		static VertexBufferHandle CreateVertexBuffer(const std::vector<PosColorVertex>& vertices);
		static void BindVertexBuffer(VertexBufferHandle handle, uint32_t stream);
		static void Destroy(VertexBufferHandle vertexBufferHandle);

		static IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices);
		static void BindIndexBuffer(IndexBufferHandle handle);
		static void Destroy(IndexBufferHandle indexBufferHandle);
	};
}
