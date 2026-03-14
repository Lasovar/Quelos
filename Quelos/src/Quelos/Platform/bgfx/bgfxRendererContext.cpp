#include "qspch.h"
#include "bgfxRendererContext.h"

#include "bgfx/bgfx.h"

namespace Quelos {
    static bgfx::RendererType::Enum GetRendererType(const RendererAPI api) {
        switch (api) {
        case RendererAPI::None: return bgfx::RendererType::Noop;
        case RendererAPI::OpenGL: return bgfx::RendererType::OpenGL;
        case RendererAPI::Vulkan: return bgfx::RendererType::Vulkan;
        case RendererAPI::Direct3D11: return bgfx::RendererType::Direct3D11;
        case RendererAPI::Direct3D12: return bgfx::RendererType::Direct3D12;
        case RendererAPI::Metal: return bgfx::RendererType::Metal;
        }

        QS_CORE_ASSERT(false, "Unknown RendererAPI");
        return bgfx::RendererType::Noop;
    }

    static ResourceTable<bgfx::VertexBufferHandle, VertexBuffer> s_VertexBufferTable;
    static ResourceTable<bgfx::IndexBufferHandle, IndexBuffer> s_IndexBufferTable;

    void bgfxRendererContext::Init(const Ref<Window>& window, const RendererAPI api) {
        bgfx::PlatformData platformData;
        platformData.nwh = window->GetNativeWindow();
        platformData.ndt = window->GetNativeDisplay();

        bgfx::Init bgfxInit;
        bgfxInit.type = GetRendererType(api);
        bgfxInit.resolution.width = window->GetWidth();
        bgfxInit.resolution.height = window->GetHeight();
        bgfxInit.resolution.reset = BGFX_RESET_NONE;
#if QUELOS_PLATFORM_WINDOWS
        platformData.type = bgfx::NativeWindowHandleType::Default;
#elif QUELOS_PLATFORM_LINUX
        platformData.type = window->IsWayland()
                                ? bgfx::NativeWindowHandleType::Wayland
                                : bgfx::NativeWindowHandleType::Default;
#endif

        bgfxInit.platformData = platformData;
        bgfx::init(bgfxInit);
    }

    VertexBufferHandle bgfxRendererContext::CreateVertexBuffer(const std::vector<PosColorVertex>& vertices) {
        bgfx::VertexLayout pvcLayout;
        pvcLayout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        bgfx::VertexBufferHandle handle = createVertexBuffer(bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosColorVertex)), pvcLayout);

        if (!bgfx::isValid(handle)) {
            QS_CORE_ERROR("Failed to create VertexBuffer!");
        }

        return s_VertexBufferTable.Emplace(handle);
    }

    void bgfxRendererContext::BindVertexBuffer(const VertexBufferHandle vertexBufferHandle, const uint32_t stream) {
        bgfx::setVertexBuffer(stream, *s_VertexBufferTable.Get(vertexBufferHandle));
    }

    void bgfxRendererContext::Destroy(const VertexBufferHandle vertexBufferHandle) {
        bgfx::destroy(*s_VertexBufferTable.Get(vertexBufferHandle));
        s_VertexBufferTable.Erase(vertexBufferHandle);
    }

    IndexBufferHandle bgfxRendererContext::CreateIndexBuffer(const std::vector<uint16_t>& indices) {
        const bgfx::IndexBufferHandle handle = bgfx::createIndexBuffer(
            bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t))
        );

        if (!bgfx::isValid(handle)) {
            QS_CORE_ERROR("Failed to create IndexBuffer!");
        }

        return s_IndexBufferTable.Emplace(handle);
    }

    void bgfxRendererContext::BindIndexBuffer(const IndexBufferHandle indexBufferHandle) {
        bgfx::setIndexBuffer(*s_IndexBufferTable.Get(indexBufferHandle));
    }

    void bgfxRendererContext::Destroy(const IndexBufferHandle indexBufferHandle) {
        bgfx::destroy(*s_IndexBufferTable.Get(indexBufferHandle));
        s_IndexBufferTable.Erase(indexBufferHandle);
    }
}
