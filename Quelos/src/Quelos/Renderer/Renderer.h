#pragma once

#include <Quelos/Scenes/Components.h>

#include "FrameBuffer.h"
#include "RendererContext.h"
#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/RendererAPI.h"
#include "Shader.h"
#include "VertexBufferLayout.h"

namespace Quelos {
	class ImGuiState;
	class Window;

	using RendererContextFactory = Ref<RendererContext>(*)();
	using ImGuiStateFactory = Ref<ImGuiState>(*)();

	struct RendererFactory {
		RendererContextFactory ContextFactory = nullptr;
		ImGuiStateFactory ImGuiStateFactory = nullptr;
	};

	class QS_API Renderer {
	public:
		static bool IsInitialized();
		static void RegisterRendererContext(RendererFactory factory);
		static void Init(const Ref<Window>& window, RendererAPI api);
		static void Shutdown();

		static void OnEvent(Event& event);

		static bool HomogenousDepth();

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

		// Texture
		static TextureHandle CreateTexture(const TextureSpecification& spec);
		static TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data);
		static TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path);

		static bool TextureIsVFlipped();

		static const TextureSpecification* GetSpecification(TextureHandle handle);
		static uint16_t TextureGetNativeHandle(TextureHandle handle);
		static void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height);

		static void Bind(TextureHandle textureHandle);
		static void Destroy(TextureHandle textureHandle);

		// Frame Buffer
		static FrameBufferHandle CreateFrameBuffer(uint32_t viewID, Span<TextureHandle> attachments);

		static uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle);
		static uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle);
		static glm::uvec2 FrameBufferGetSize(FrameBufferHandle frameBufferHandle);

		static void FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId);
		static uint32_t FrameBufferGetViewID(FrameBufferHandle frameBufferHandle);

		static void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height);

		static void Bind(FrameBufferHandle frameBufferHandle);
		static void Destroy(FrameBufferHandle frameBufferHandle);
	};
}
