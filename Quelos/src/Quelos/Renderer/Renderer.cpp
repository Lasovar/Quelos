#include "Renderer.h"
#include <bgfx/bgfx.h>

#include "Quelos/Core/Window.h"

#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"
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
    static uint32_t s_CurrentViewID = BGFX_INVALID_HANDLE;
    static bool s_IsInitialized = false;

    static bgfx::RendererType::Enum GetRendererType(const RendererAPI api) {
        switch (api) {
            case RendererAPI::None:        return bgfx::RendererType::Noop;
            case RendererAPI::OpenGL:      return bgfx::RendererType::OpenGL;
            case RendererAPI::Vulkan:      return bgfx::RendererType::Vulkan;
            case RendererAPI::Direct3D11:  return bgfx::RendererType::Direct3D11;
            case RendererAPI::Direct3D12:  return bgfx::RendererType::Direct3D12;
            case RendererAPI::Metal:       return bgfx::RendererType::Metal;
        }

        QS_CORE_ASSERT(false, "Unknown RendererAPI");
        return bgfx::RendererType::Noop;
    }

    bool Renderer::IsInitialized() { return s_IsInitialized; }

    void Renderer::Init(const Ref<Window>& window, const RendererAPI api) {
        s_Window = window;
        s_Time = Application::Get().GetTime();

        bgfx::PlatformData platformData;
        platformData.nwh = window->GetNativeWindow();
        platformData.ndt = window->GetNativeDisplay();

        bgfx::Init bgfxInit;
        bgfxInit.type = GetRendererType(api);
        bgfxInit.resolution.width = window->GetWidth();
        bgfxInit.resolution.height = window->GetHeight();
        bgfxInit.resolution.reset = BGFX_RESET_NONE;
#if QUELOS_PLATFORM_WINDOWS
        // TODO: Try to get Direct3D12 to work
        platformData.type = bgfx::NativeWindowHandleType::Default;
#elif QUELOS_PLATFORM_LINUX
        // TODO: SETUP VULKAN PROPERLY ON WAYLAND
        platformData.type = s_Window->IsWayland()
                                ? bgfx::NativeWindowHandleType::Wayland
                                : bgfx::NativeWindowHandleType::Default;
#endif

        bgfxInit.platformData = platformData;
        bgfx::init(bgfxInit);

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

    void Renderer::StartSceneRender(const Ref<FrameBuffer>& frameBuffer, const glm::mat4& view, const glm::mat4& projection) {
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

        mesh.MeshData->GetVertexBuffer()->Bind(0);
        mesh.MeshData->GetIndexBuffer()->Bind();

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
}
