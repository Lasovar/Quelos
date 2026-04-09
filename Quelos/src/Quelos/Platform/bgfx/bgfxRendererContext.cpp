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
            const Path& assets = "Assets/";

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

            if (const Buffer data = ReadFile(shaderPath / fileName); data) {
                const bgfx::Memory* mem = bgfx::copy(data.GetData(), data.GetSize());

                const bgfx::ShaderHandle handle = bgfx::createShader(mem);
                bgfx::setName(handle, fileName.c_str(), static_cast<int32_t>(fileName.length()));

                return handle;
            }

            return BGFX_INVALID_HANDLE;
        }

        bgfx::AttribType::Enum ToBGFX(const ShaderDataType type) {
            switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
                return bgfx::AttribType::Float;

            case ShaderDataType::UNorm8x4:
            case ShaderDataType::UNorm8x2:
                return bgfx::AttribType::Uint8;
            case ShaderDataType::UInt10x3_A2:
                return bgfx::AttribType::Uint10;

            case ShaderDataType::UNorm16x2:
            case ShaderDataType::UNorm16x4:
                return bgfx::AttribType::Int16;

            default:
                return bgfx::AttribType::Float;
            }
        }

        constexpr bgfx::Attrib::Enum ToBGFX(VertexAttribute attr) {
            return static_cast<bgfx::Attrib::Enum>(attr);
        }

        static bgfx::VertexLayout ToBGFXLayout(const VertexLayout& layout) {
            bgfx::VertexLayout result;
            result.begin();

            for (uint8_t i = 0; i < layout.Count; ++i) {
                const auto& element = layout[i];

                result.add(
                    ToBGFX(element.Attribute),
                    ComponentCount(element.Type),
                    ToBGFX(element.Type),
                    IsNormalized(element.Type),
                    IsIntegerType(element.Type)
                );
            }

            result.end();
            return result;
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
#if QS_PLATFORM_WINDOWS
        platformData.type = bgfx::NativeWindowHandleType::Default;
#elif QS_PLATFORM_LINUX
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
            return { ShaderHandle::Invalid };
        }

        if (!bgfx::isValid(fsh)) {
            bgfx::destroy(vsh);

            QS_CORE_ERROR("Fragment shader '{}' failed to load", filePathFragment);
            return { ShaderHandle::Invalid };
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

    VertexBufferHandle bgfxRendererContext::CreateVertexBuffer(const BufferView vertices, const VertexLayout bufferLayout) {
        const bgfx::VertexLayout pvcLayout = Utility::ToBGFXLayout(bufferLayout);

        bgfx::VertexBufferHandle handle = createVertexBuffer(
            bgfx::makeRef(vertices.data(), vertices.size()),
            pvcLayout
        );

        if (!bgfx::isValid(handle)) {
            QS_CORE_ERROR("Failed to create VertexBuffer!");
            return { VertexBufferHandle::Invalid };
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
            return { IndexBufferHandle::Invalid };
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
