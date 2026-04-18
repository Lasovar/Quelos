#include "Renderer.h"

#include "Quelos/Core/Window.h"

#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "RendererContext.h"
#include "Shader.h"
#include "VertexBuffer.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/WindowEvents.h"
#include "Quelos/ImGui/ImGuiState.h"

#include "Quelos/Math/Math.h"

namespace Quelos {
    static Ref<Window> s_Window;
    static Ref<Time> s_Time;

    static bool s_NeedReset = false;
    static bool s_IsInitialized = false;

    static Ref<RendererContext> s_RendererContext;

    static RendererFactory s_RendererContextFactory;

    bool Renderer::IsInitialized() { return s_IsInitialized; }

    void Renderer::RegisterRendererContext(const RendererFactory factory) {
        s_RendererContextFactory = factory;
        ImGuiState::Register(s_RendererContextFactory.ImGuiStateFactory);
    }

    void Renderer::Init(const Ref<Window>& window, const RendererAPI api) {
        s_Window = window;
        s_Time = Application::Get().GetTime();

        if (!s_RendererContextFactory.ContextFactory) {
            return;
        }

        s_RendererContext = s_RendererContextFactory.ContextFactory();
        if (!s_RendererContext) {
            return;
        }

        s_RendererContext->Init(window, api);

        s_IsInitialized = true;
    }

    void Renderer::StartFrame() {
        if (s_NeedReset) {
            s_RendererContext->Reset(s_Window->GetWidth(), s_Window->GetHeight());
            s_NeedReset = false;
        }

        s_RendererContext->StartFrame();
    }

    void Renderer::StartSceneRender(
        const Ref<FrameBuffer>& frameBuffer,
        const WorldTransform& transform,
        const glm::mat4& projection
    ) {
        StartSceneRender(frameBuffer, Math::ViewMatrix(transform.Value), projection);
    }

    void Renderer::StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const glm::mat4& view,
                                    const glm::mat4& projection) {
        s_RendererContext->StartSceneRender(frameBuffer->GetHandle(), view, projection);
    }

    void Renderer::EndFrame() {
        QS_PROFILE_SCOPED();
        s_RendererContext->EndFrame();
    }

    void Renderer::SubmitMesh(const uint32_t viewID, const MeshComponent& mesh, const WorldTransform& transform) {
        s_RendererContext->SubmitMesh(viewID, mesh, transform);
    }

    void Renderer::Shutdown() {
        s_RendererContext->Shutdown();
        s_IsInitialized = false;
    }

    void Renderer::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([](const WindowResizeEvent& _) {
            s_NeedReset = true;
            return false;
        });
    }

    bool Renderer::HomogenousDepth() {
        return s_RendererContext->HomogenousDepth();
    }

    ShaderHandle Renderer::CreateShader(const std::string& filePathVertex, const std::string& filePathFragment) {
        return s_RendererContext->CreateShader(filePathVertex, filePathFragment);
    }

    void Renderer::Submit(const ShaderHandle handle, const uint32_t view) {
        s_RendererContext->Submit(handle, view);
    }

    void Renderer::Destroy(const ShaderHandle shaderHandle) {
        s_RendererContext->Destroy(shaderHandle);
    }

    VertexBufferHandle Renderer::CreateVertexBuffer(const BufferView vertices, const VertexLayout& bufferLayout) {
        return s_RendererContext->CreateVertexBuffer(vertices, bufferLayout);
    }

    void Renderer::BindVertexBuffer(const VertexBufferHandle handle, const uint32_t stream) {
        s_RendererContext->BindVertexBuffer(handle, stream);
    }

    void Renderer::Destroy(const VertexBufferHandle vertexBufferHandle) {
        s_RendererContext->Destroy(vertexBufferHandle);
    }

    IndexBufferHandle Renderer::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
        return s_RendererContext->CreateIndexBuffer(indices);
    }

    void Renderer::BindIndexBuffer(const IndexBufferHandle handle) {
        s_RendererContext->BindIndexBuffer(handle);
    }

    void Renderer::Destroy(const IndexBufferHandle indexBufferHandle) {
        s_RendererContext->Destroy(indexBufferHandle);
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec) {
        return s_RendererContext->CreateTexture(spec);
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec, Buffer data) {
        return s_RendererContext->CreateTexture(spec, std::move(data));
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec, const OsPath& path) {
        return s_RendererContext->CreateTexture(spec, path);
    }

    bool Renderer::TextureIsVFlipped() {
        return s_RendererContext->TextureIsVFlipped();
    }

    const TextureSpecification* Renderer::GetSpecification(const TextureHandle handle) {
        return s_RendererContext->GetSpecification(handle);
    }

    uint16_t Renderer::TextureGetNativeHandle(const TextureHandle handle) {
        return s_RendererContext->TextureGetNativeHandle(handle);
    }

    void Renderer::TextureResize(const TextureHandle textureHandle, const uint32_t width, const uint32_t height) {
        s_RendererContext->TextureResize(textureHandle, width, height);
    }

    void Renderer::Bind(const TextureHandle textureHandle) {
        s_RendererContext->Bind(textureHandle);
    }

    void Renderer::Destroy(const TextureHandle textureHandle) {
        s_RendererContext->Destroy(textureHandle);
    }

    FrameBufferHandle Renderer::CreateFrameBuffer(uint32_t viewID, Span<TextureHandle> attachments) {
        return s_RendererContext->CreateFrameBuffer(viewID, attachments);
    }

    uint32_t Renderer::FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetWidth(frameBufferHandle);
    }

    uint32_t Renderer::FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetHeight(frameBufferHandle);
    }

    glm::uvec2 Renderer::FrameBufferGetSize(FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetSize(frameBufferHandle);
    }

    void Renderer::FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId) {
        s_RendererContext->FrameBufferSetViewID(frameBufferHandle, viewId);
    }

    uint32_t Renderer::FrameBufferGetViewID(FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetViewID(frameBufferHandle);
    }

    void Renderer::FrameBufferResize(const FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) {
        s_RendererContext->FrameBufferResize(frameBufferHandle, width, height);
    }

    void Renderer::Bind(const FrameBufferHandle frameBufferHandle) {
        s_RendererContext->Bind(frameBufferHandle);
    }

    void Renderer::Destroy(const FrameBufferHandle frameBufferHandle) {
        s_RendererContext->Destroy(frameBufferHandle);
    }
}
