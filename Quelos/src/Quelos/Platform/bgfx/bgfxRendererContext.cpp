#include "qspch.h"
#include "bgfxRendererContext.h"

#include "bgfx/bgfx.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Renderer/Shader.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    namespace Utility {
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

        static bgfx::ShaderHandle LoadShader(const std::string& fileName) {
            Path shaderPath;
            const Path& assets = Project::GetAssetsPath();

            switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Noop:
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12: shaderPath = assets / "shaders/dx11/"; break;
            case bgfx::RendererType::Gnm: shaderPath = assets / "shaders/pssl/"; break;
            case bgfx::RendererType::Metal: shaderPath = assets / "shaders/metal/"; break;
            case bgfx::RendererType::OpenGL: shaderPath = assets / "shaders/glsl/"; break;
            case bgfx::RendererType::OpenGLES: shaderPath = assets / "shaders/essl/"; break;
            case bgfx::RendererType::Vulkan: shaderPath = assets / "shaders/spirv/"; break;
            case bgfx::RendererType::Agc:
            case bgfx::RendererType::Nvn:
            case bgfx::RendererType::WebGPU:
            case bgfx::RendererType::Count:
                break;
            }

            if (const std::vector<byte> data = ReadBinaryFile(shaderPath / fileName); !data.empty()) {
                const bgfx::Memory* mem = bgfx::copy(data.data(), data.size());

                const bgfx::ShaderHandle handle = bgfx::createShader(mem);
                bgfx::setName(handle, fileName.c_str(), static_cast<int32_t>(fileName.length()));

                return handle;
            }

            return BGFX_INVALID_HANDLE;
        }
    }

    static ResourceTable<bgfx::VertexBufferHandle, VertexBuffer> s_VertexBufferTable;
    static ResourceTable<bgfx::IndexBufferHandle, IndexBuffer> s_IndexBufferTable;
    static ResourceTable<bgfx::ProgramHandle, Shader> s_ShaderTable;

    void bgfxRendererContext::Init(const Ref<Window>& window, const RendererAPI api) {
        bgfx::PlatformData platformData;
        platformData.nwh = window->GetNativeWindow();
        platformData.ndt = window->GetNativeDisplay();

        bgfx::Init bgfxInit;
        bgfxInit.type = Utility::GetRendererType(api);
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

    void bgfxRendererContext::Shutdown() {
        for (auto&& vertexBufferHandle : s_VertexBufferTable.GetAllHandles()) {
            Destroy(vertexBufferHandle);
        }

        for (auto&& indexBufferHandle : s_IndexBufferTable.GetAllHandles()) {
            Destroy(indexBufferHandle);
        }

        for (auto&& shaderHandle : s_ShaderTable.GetAllHandles()) {
            Destroy(shaderHandle);
        }

        bgfx::shutdown();
    }

    ShaderHandle bgfxRendererContext::CreateShader(const std::string& filePathVertex,
        const std::string& filePathFragment) {

        const bgfx::ShaderHandle vsh = Utility::LoadShader(filePathVertex);
        const bgfx::ShaderHandle fsh = Utility::LoadShader(filePathFragment);

        if (!bgfx::isValid(vsh)) {
            QS_CORE_ERROR("Vertex shader '{}' failed to load", filePathVertex);
            return ShaderHandle(ShaderHandle::Invalid);
        }

        if (!bgfx::isValid(fsh)) {
            bgfx::destroy(vsh);

            QS_CORE_ERROR("Fragment shader '{}' failed to load", filePathFragment);
            return ShaderHandle(ShaderHandle::Invalid);
        }

        bgfx::ProgramHandle handle = bgfx::createProgram(vsh, fsh, true);
        return s_ShaderTable.Emplace(handle);
    }

    void bgfxRendererContext::Submit(const ShaderHandle shaderHandle, const uint32_t view) {
        bgfx::submit(view, *s_ShaderTable.Get(shaderHandle));
    }

    void bgfxRendererContext::Destroy(const ShaderHandle shaderHandle) {
        bgfx::destroy(*s_ShaderTable.Get(shaderHandle));
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
