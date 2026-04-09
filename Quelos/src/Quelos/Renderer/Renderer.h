#pragma once

#include <Quelos/Scenes/Components.h>

#include "FrameBuffer.h"
#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/RendererAPI.h"
#include "Shader.h"
#include "VertexBufferLayout.h"

namespace Quelos {
	class Window;

	class QS_API Renderer {
	public:
		static bool IsInitialized();
		static void Init(const Ref<Window>& window, RendererAPI api);
		static void Shutdown();

		static void OnEvent(Event& event);

		static void StartFrame();
		static void StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const WorldTransform& transform, const glm::mat4& projection);
		static void StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const glm::mat4& view, const glm::mat4& projection);
		static void EndFrame();

		static void SubmitMesh(uint32_t viewID, const MeshComponent& mesh, const WorldTransform& transform);

		// Renderer Context API
        static ShaderHandle CreateShader(const std::string& filePathVertex, const std::string& filePathFragment);
		static void Submit(ShaderHandle handle, uint32_t view);
		static void Destroy(ShaderHandle shaderHandle);

		static VertexBufferHandle CreateVertexBuffer(BufferView vertices, const VertexLayout& bufferLayout);
		static void BindVertexBuffer(VertexBufferHandle handle, uint32_t stream);
		static void Destroy(VertexBufferHandle vertexBufferHandle);

		static IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices);
		static void BindIndexBuffer(IndexBufferHandle handle);
		static void Destroy(IndexBufferHandle indexBufferHandle);
	};
}
