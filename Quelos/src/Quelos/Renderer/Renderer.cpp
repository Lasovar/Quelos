#include <qspch.h>
#include "Renderer.h"
#include <bgfx/bgfx.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Quelos/Core/Window.h"
#include "bx/math.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include "IndexBuffer.h"
#include "Material.h"
#include "Shader.h"
#include "VertexBuffer.h"

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/WindowEvents.h"

namespace Quelos {
    static Ref<Window> s_Window;
    static Ref<Time> s_Time;

    static bool s_NeedReset = false;

    void Renderer::Init(const Ref<Window>& window) {
        s_Window = window;
        s_Time = Application::Get().GetTime();

        bgfx::PlatformData platformData;
        platformData.nwh = window->GetNativeWindow();
        platformData.ndt = window->GetNativeDisplay();

        bgfx::Init bgfxInit;
        bgfxInit.type = bgfx::RendererType::Count;
        bgfxInit.resolution.width = window->GetWidth();
        bgfxInit.resolution.height = window->GetHeight();
        bgfxInit.resolution.reset = BGFX_RESET_NONE;
#if QUELOS_PLATFORM_WINDOWS
        // TODO: Try to get Direct3D12 to work
        bgfxInit.type = bgfx::RendererType::Direct3D11;
        platformData.type = bgfx::NativeWindowHandleType::Default;
#elif QUELOS_PLATFORM_LINUX
        // TODO: SETUP VULKAN PROPERLY ON WAYLAND
        bgfxInit.type = bgfx::RendererType::OpenGL;
        platformData.type = bgfx::NativeWindowHandleType::Wayland;
#endif

        bgfxInit.platformData = platformData;
        bgfx::init(bgfxInit);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    }

    void Renderer::StartFrame() {
        if (s_NeedReset) {
            bgfx::reset(s_Window->GetWidth(), s_Window->GetHeight(), BGFX_RESET_NONE);
            s_NeedReset = false;
        }

        bgfx::setViewRect(0, 0, 0, s_Window->GetWidth(), s_Window->GetHeight());
        bgfx::touch(0);
    }

    void Renderer::StartSceneRender(const CameraComponent& Camera, const TransformComponent& CameraTransform) {
        glm::mat4 view;

        bx::mtxFromQuaternion(glm::value_ptr(view),
                              bx::Quaternion(
                                  CameraTransform.Rotation.x,
                                  CameraTransform.Rotation.y,
                                  CameraTransform.Rotation.z,
                                  CameraTransform.Rotation.w
                              ),
                              bx::Vec3(
                                  CameraTransform.Position.x,
                                  CameraTransform.Position.y,
                                  CameraTransform.Position.z
                              )
        );

        glm::mat4 proj;
        bx::mtxProj(
            glm::value_ptr(proj),
            Camera.FOV,
            static_cast<float>(s_Window->GetWidth()) / s_Window->GetHeight(), // NOLINT(*-narrowing-conversions)
            Camera.Near,
            Camera.Far,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));
    }

    void Renderer::EndFrame() {
        bgfx::frame();
    }

    void Renderer::SubmitMesh(const MeshComponent& mesh, const TransformComponent& transform) {
        glm::mat4 mat;
        bx::mtxFromQuaternion(glm::value_ptr(mat),
                              bx::Quaternion(
                                  transform.Rotation.x,
                                  transform.Rotation.y,
                                  transform.Rotation.z,
                                  transform.Rotation.w
                              )
        );

        mat[3][0] = transform.Position.x;
        mat[3][1] = transform.Position.y;
        mat[3][2] = transform.Position.z;

        bgfx::setTransform(glm::value_ptr(mat));

        bgfx::setVertexBuffer(0, mesh.VertexBuffer->GetHandle());
        bgfx::setIndexBuffer(mesh.IndexBuffer->GetHandle());

        bgfx::submit(0, mesh.Material->GetShader()->GetHandle());
    }

    void Renderer::Shutdown() {
        bgfx::shutdown();
    }

    void Renderer::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([](const WindowResizeEvent& e) {
            s_NeedReset = true;
            return false;
        });
    }
}
