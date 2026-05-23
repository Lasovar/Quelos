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
		static void StartSceneRender(const float4x4& view, const float4x4& projection);
		static void BeginRenderPass(const BeginRenderPassAttribs& attribs);
		static void EndRenderPass();
		static void EndFrame();

		static void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform);

		// Renderer Context API
        static ShaderHandle CreateShader(Buffer vertex, Buffer fragment, const std::string& name);
        static bool RecreateShader(ShaderHandle handle, Buffer vertex, Buffer fragment);
		static void Submit(ShaderHandle handle, uint32_t view);
		static void Destroy(ShaderHandle shaderHandle);

		static VertexBufferHandle CreateVertexBuffer(BufferView vertices, const VertexLayout& bufferLayout);
		static void BindVertexBuffer(VertexBufferHandle handle, uint32_t stream);
		static void Destroy(VertexBufferHandle vertexBufferHandle);

		static IndexBufferHandle CreateIndexBuffer(Span<uint16_t> indices);
		static void BindIndexBuffer(IndexBufferHandle handle);
		static void Destroy(IndexBufferHandle indexBufferHandle);

		static UniformBufferHandle CreateUniformBuffer(const std::string& name, UniformBufferType uniformType, uint32_t count = 1);
		static void SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data, uint32_t count = 1);
		static void Destroy(UniformBufferHandle uniformBufferHandle);

		// Texture
		static TextureHandle CreateTexture(const TextureSpecification& spec);
		static TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data);
		static TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path);

		static bool TextureIsVFlipped();

		static const TextureSpecification* GetSpecification(TextureHandle handle);
		static uint64_t TextureGetNativeHandle(TextureHandle handle);
		static void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height);

		static void Bind(TextureHandle textureHandle);
		static void Destroy(TextureHandle textureHandle);

		// Render Pass
		static RenderPassHandle CreateRenderPass(const RenderPassSpec& renderPassSpec);

		// Frame Buffer
		static FrameBufferHandle CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec);

		static uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle);
		static uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle);
		static Extent2D FrameBufferGetSize(FrameBufferHandle frameBufferHandle);

		static void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height);

		static void Destroy(FrameBufferHandle frameBufferHandle);
	};
}
