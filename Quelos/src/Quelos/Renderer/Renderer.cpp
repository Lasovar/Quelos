#include "Renderer.h"
#include <bgfx/bgfx.h>

#include "Quelos/Core/Window.h"

#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "RendererContext.h"
#include "Shader.h"
#include "VertexBuffer.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/WindowEvents.h"

#include "Quelos/Math/Math.h"

namespace Quelos {
    static Ref<Window> s_Window;
    static Ref<Time> s_Time;

    static bool s_NeedReset = false;
    static bool s_IsInitialized = false;

    static Ref<RendererContext> s_RendererContext;

    bool Renderer::IsInitialized() { return s_IsInitialized; }

    void Renderer::Init(const Ref<Window>& window, const RendererAPI api) {
        s_Window = window;
        s_Time = Application::Get().GetTime();

        s_RendererContext = RendererContext::Create();
        s_RendererContext->Init(window, api);

        s_IsInitialized = true;
    }

    void Renderer::StartFrame() {
        if (s_NeedReset) {
            bgfx::reset(s_Window->GetWidth(), s_Window->GetHeight(), BGFX_RESET_NONE);
            s_NeedReset = false;
        }
    }

    void Renderer::StartSceneRender(
        const Ref<FrameBuffer>& frameBuffer,
        const TransformComponent& transform,
        const glm::mat4& projection
    ) {
        StartSceneRender(frameBuffer, Math::ViewMatrix(transform.Rotation, transform.Position), projection);
    }

    void Renderer::StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const glm::mat4& view,
                                    const glm::mat4& projection) {
        const uint32_t viewId = frameBuffer->GetViewID();
        frameBuffer->Bind();

        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);

        bgfx::setViewRect(viewId, 0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight());
        bgfx::touch(viewId);

        bgfx::setViewTransform(viewId, glm::value_ptr(view), glm::value_ptr(projection));
    }

    void Renderer::EndFrame() {
        bgfx::frame();
    }

    void Renderer::SubmitMesh(const uint32_t viewID, const MeshComponent& mesh, const TransformComponent& transform) {
        glm::mat4 mat = Math::SRTMatrix(transform.Scale, transform.Rotation, transform.Position);

        // Move functionality to Uniform Buffers
        bgfx::setTransform(glm::value_ptr(mat));

        mesh.MeshData->GetVertexBuffer().Bind(0);
        mesh.MeshData->GetIndexBuffer().Bind();

        mesh.MaterialData->GetShader()->Submit(viewID);
    }

    void Renderer::Shutdown() {
        bgfx::shutdown();
    }

    void Renderer::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([](const WindowResizeEvent& _) {
            s_NeedReset = true;
            return false;
        });
    }

    VertexBufferHandle Renderer::CreateVertexBuffer(const std::vector<PosColorVertex>& vertices) {
        return s_RendererContext->CreateVertexBuffer(vertices);
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
}
