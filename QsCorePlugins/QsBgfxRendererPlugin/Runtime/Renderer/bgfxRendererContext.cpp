#include "qspch.h"
#include "bgfxRendererContext.h"

#include "bgfx/bgfx.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Renderer/Shader.h"
#include "Quelos/Renderer/Texture.h"
#include "Quelos/Utility/QuelosUtil.h"


#include <bx/file.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>

#include "glm/gtc/type_ptr.hpp"
#include "Quelos/Renderer/Renderer.h"

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

        static bgfx::ShaderHandle LoadShader(Buffer shader, const std::string& name) {
            const bgfx::Memory* mem = bgfx::makeRef(
                shader.data(), shader.size(),
                [](void* data, void* userData) { std::bit_cast<Buffer::Deleter>(userData)(data); },
                std::bit_cast<void*>(shader.deleter())
            );

            shader.release_ownership();

            const bgfx::ShaderHandle handle = bgfx::createShader(mem);
            bgfx::setName(handle, name.c_str(), static_cast<int32_t>(name.length()));

            return handle;
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


    struct TextureData {
        bgfx::TextureHandle Handle = BGFX_INVALID_HANDLE;
        TextureSpecification Specification;
    };

    namespace TextureUtil {
        static void ImageReleaseCb(void* pointer, void* userData) {
            BX_UNUSED(pointer);
            const auto imageContainer = static_cast<bimg::ImageContainer*>(userData);
            bimg::imageFree(imageContainer);
        }

        static bgfx::TextureHandle LoadTexture(const std::filesystem::path& filePath, const uint64_t flags,
                                               bgfx::TextureInfo* info, bimg::Orientation::Enum* orientation) {
            bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

            bx::DefaultAllocator allocator;
            if (const Buffer data = Utility::ReadFile(filePath); !data) {
                bimg::ImageContainer* imageContainer = bimg::imageParse(&allocator, data.data(), data.size());
                if (imageContainer != nullptr) {
                    if (orientation) {
                        *orientation = imageContainer->m_orientation;
                    }

                    const bgfx::Memory* mem = bgfx::makeRef(
                        imageContainer->m_data,
                        imageContainer->m_size,
                        ImageReleaseCb,
                        &imageContainer
                    );

                    if (info != nullptr) {
                        bgfx::calcTextureSize(
                            *info,
                            static_cast<uint16_t>(imageContainer->m_width),
                            static_cast<uint16_t>(imageContainer->m_height),
                            static_cast<uint16_t>(imageContainer->m_depth),
                            imageContainer->m_cubeMap,
                            imageContainer->m_numMips > 1,
                            imageContainer->m_numLayers,
                            static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format)
                        );
                    }

                    if (imageContainer->m_cubeMap) {
                        handle = bgfx::createTextureCube(
                            static_cast<uint16_t>(imageContainer->m_width),
                            1 < imageContainer->m_numMips,
                            imageContainer->m_numLayers,
                            static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format),
                            flags,
                            mem
                        );
                    }
                    else if (imageContainer->m_depth > 1) {
                        handle = bgfx::createTexture3D(
                            static_cast<uint16_t>(imageContainer->m_width),
                            static_cast<uint16_t>(imageContainer->m_height),
                            static_cast<uint16_t>(imageContainer->m_depth),
                            1 < imageContainer->m_numMips,
                            static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format),
                            flags,
                            mem
                        );
                    }
                    else if (bgfx::isTextureValid(0, false, imageContainer->m_numLayers,
                                                  static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format),
                                                  flags)) {
                        handle = bgfx::createTexture2D(
                            static_cast<uint16_t>(imageContainer->m_width),
                            static_cast<uint16_t>(imageContainer->m_height),
                            1 < imageContainer->m_numMips,
                            imageContainer->m_numLayers,
                            static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format),
                            flags,
                            mem
                        );
                    }

                    if (bgfx::isValid(handle)) {
                        const bx::StringView name(filePath.filename().string().c_str());
                        bgfx::setName(handle, name.getPtr(), name.getLength());
                    }
                }
            }

            return handle;
        }

        static bgfx::TextureFormat::Enum QuelosImageFormatToBgfxFormat(const ImageFormat format) {
            switch (format) {
            case ImageFormat::RGBA: return bgfx::TextureFormat::RGBA8;
            case ImageFormat::RGBA16F: return bgfx::TextureFormat::RGBA16F;
            case ImageFormat::RGB: return bgfx::TextureFormat::RGB8;
            case ImageFormat::RED8UN: return bgfx::TextureFormat::R8;
            case ImageFormat::DEPTH32F: return bgfx::TextureFormat::D32F;
            case ImageFormat::Depth: return bgfx::TextureFormat::D24S8;
            default:
                QS_CORE_WARN("Unsupported image format conversion to bgfx format!");
                return bgfx::TextureFormat::Unknown;
            }
        }

        static uint64_t ToBgfxSamplerFlags(const TextureWrap wrap) {
            switch (wrap) {
            case TextureWrap::Clamp:
                return BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
            case TextureWrap::Repeat:
                return 0;
            default:
                QS_CORE_WARN("Unsupported texture wrap conversion to bgfx sampler flags!");
                return 0;
            }
        }

        static uint64_t ToBgfxSamplerFlags(const TextureFilter filter) {
            switch (filter) {
            case TextureFilter::Linear:
                return 0;
            case TextureFilter::Nearest:
                return BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
            case TextureFilter::Anisotropic:
                return BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
            default:
                QS_CORE_WARN("Unsupported texture filter conversion to bgfx sampler flags!");
                return 0;
            }
        }

        static uint64_t GetBgfxRenderTextureMSAA(const RenderTargetMSAA textureMsaa) {
            switch (textureMsaa) {
            case RenderTargetMSAA::None: return 0;
            case RenderTargetMSAA::MSAA_X2: return BGFX_TEXTURE_RT_MSAA_X2;
            case RenderTargetMSAA::MSAA_X4: return BGFX_TEXTURE_RT_MSAA_X4;
            case RenderTargetMSAA::MSAA_X8: return BGFX_TEXTURE_RT_MSAA_X8;
            case RenderTargetMSAA::MSAA_X16: return BGFX_TEXTURE_RT_MSAA_X16;
            }

            return 0;
        }

        static uint64_t ToBgfxTextureFlags(const TextureSpecification& spec) {
            uint64_t flags = ToBgfxSamplerFlags(spec.SamplerWrap) | ToBgfxSamplerFlags(spec.SamplerFilter);
            if (spec.RenderTarget != TextureRenderTarget::Off) {
                if (spec.RenderTarget == TextureRenderTarget::ReadWrite) {
                    flags |= BGFX_TEXTURE_RT;
                }
                else if (spec.RenderTarget == TextureRenderTarget::WriteOnly) {
                    flags |= BGFX_TEXTURE_RT_WRITE_ONLY;
                }

                flags |= GetBgfxRenderTextureMSAA(spec.MSAAType);
            }


            if (spec.IsBlitDestination) {
                flags |= BGFX_TEXTURE_BLIT_DST;
            }

            return flags;
        }

        struct Payload {
            Buffer::Deleter Deleter;
        };

        static bgfx::TextureHandle CreateTextureHandle(const TextureSpecification& spec, Buffer data = {}) {
            const bgfx::Memory* mem = nullptr;

            if (data) {
                mem = bgfx::makeRef(data.data(), data.size(), [](void* data, void* userData) {
                    std::bit_cast<Buffer::Deleter>(userData)(data);
                }, std::bit_cast<void*>(data.deleter()));

                data.release_ownership();
            }

            return bgfx::createTexture2D(
                static_cast<uint16_t>(spec.Width),
                static_cast<uint16_t>(spec.Height),
                false,
                1,
                QuelosImageFormatToBgfxFormat(spec.Format),
                ToBgfxTextureFlags(spec),
                mem
            );
        }

        static void Resize(TextureData* textureData, const uint32_t width, const uint32_t height) {
            if (bgfx::isValid(textureData->Handle)) {
                bgfx::destroy(textureData->Handle);
            }

            textureData->Specification.Width = width;
            textureData->Specification.Height = height;

            textureData->Handle = CreateTextureHandle(textureData->Specification);
        }
    }

    struct FrameBufferData {
        bgfx::FrameBufferHandle Handle = BGFX_INVALID_HANDLE;
        SmallVec<TextureHandle, 4> Attachments;
        uint32_t ViewId = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    struct ShaderData {
        bgfx::ProgramHandle Handle = BGFX_INVALID_HANDLE;
        std::string Name;
    };

    static ResourceTable<bgfx::VertexBufferHandle, VertexBuffer> s_VertexBufferTable;
    static ResourceTable<bgfx::IndexBufferHandle, IndexBuffer> s_IndexBufferTable;
    static ResourceTable<ShaderData, Shader> s_ShaderTable;
    static ResourceTable<TextureData, Texture> s_TextureDataTable;
    static ResourceTable<FrameBufferData, FrameBuffer> s_FrameBufferTable;

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

    bool bgfxRendererContext::HomogenousDepth() {
        return bgfx::getCaps()->homogeneousDepth;
    }

    void bgfxRendererContext::StartFrame() {
    }

    void bgfxRendererContext::EndFrame() {
        bgfx::frame();
    }

    void bgfxRendererContext::StartSceneRender(
        const FrameBufferHandle frameBuffer,
        const glm::mat4& view,
        const glm::mat4& projection
    ) {
        const auto* frameBufferData = s_FrameBufferTable.Get(frameBuffer);
        const uint32_t viewId = frameBufferData->ViewId;
        bgfx::setViewFrameBuffer(viewId, frameBufferData->Handle);

        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);

        bgfx::setViewRect(viewId, 0, 0, frameBufferData->Width, frameBufferData->Height);
        bgfx::touch(viewId);

        bgfx::setViewTransform(viewId, glm::value_ptr(view), glm::value_ptr(projection));
    }

    void bgfxRendererContext::SubmitMesh(const uint32_t viewID, const MeshComponent& mesh,
                                         const WorldTransform& transform) {
        bgfx::setTransform(glm::value_ptr(transform.Value));

        BindVertexBuffer(mesh.MeshData->GetVertexBuffer(), 0);
        BindIndexBuffer(mesh.MeshData->GetIndexBuffer());

        Submit(mesh.MaterialData->GetShaderHandle(), viewID);
    }

    void bgfxRendererContext::Reset(const uint32_t width, const uint32_t height) {
        bgfx::reset(width, height, BGFX_RESET_NONE);
    }

    void bgfxRendererContext::Shutdown() {
        bgfx::shutdown();
    }

    ShaderHandle bgfxRendererContext::CreateShader(Buffer vertex, Buffer fragment, const std::string& name) {
        const bgfx::ShaderHandle vsh = Utility::LoadShader(std::move(vertex), name);
        const bgfx::ShaderHandle fsh = Utility::LoadShader(std::move(fragment), name);

        if (!bgfx::isValid(vsh)) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::CreateShader",
                "Vertex shader '{}' failed to load",
                name
            );

            return {ShaderHandle::Invalid};
        }

        if (!bgfx::isValid(fsh)) {
            bgfx::destroy(vsh);

            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::CreateShader",
                "Fragment shader '{}' failed to load",
                name
            );

            return {ShaderHandle::Invalid};
        }

        bgfx::ProgramHandle handle = bgfx::createProgram(vsh, fsh, true);
        return s_ShaderTable.Emplace(ShaderData{handle, name});
    }

    bool bgfxRendererContext::RecreateShader(const ShaderHandle handle, Buffer vertex, Buffer fragment) {
        auto* shader = s_ShaderTable.Get(handle);

        const bgfx::ShaderHandle vsh = Utility::LoadShader(std::move(vertex), shader->Name);
        const bgfx::ShaderHandle fsh = Utility::LoadShader(std::move(fragment), shader->Name);

        if (!bgfx::isValid(vsh)) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::RecreateShader",
                "Vertex shader '{}' failed to load",
                shader->Name
            );

            return false;
        }

        if (!bgfx::isValid(fsh)) {
            bgfx::destroy(vsh);

            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::RecreateShader",
                "Fragment shader '{}' failed to load",
                shader->Name
            );

            return false;
        }

        const bgfx::ProgramHandle bgfxHandle = bgfx::createProgram(vsh, fsh, true);

        if (bgfx::isValid(shader->Handle)) {
            bgfx::destroy(shader->Handle);
        }

        shader->Handle = bgfxHandle;
        return true;
    }

    void bgfxRendererContext::Submit(const ShaderHandle shaderHandle, const uint32_t view) {
        bgfx::submit(view, s_ShaderTable.Get(shaderHandle)->Handle);
    }

    void bgfxRendererContext::Destroy(const ShaderHandle shaderHandle) {
        bgfx::destroy(s_ShaderTable.Get(shaderHandle)->Handle);
        s_ShaderTable.Erase(shaderHandle);
    }

    VertexBufferHandle bgfxRendererContext::CreateVertexBuffer(const BufferView vertices,
                                                               const VertexLayout bufferLayout) {
        const bgfx::VertexLayout pvcLayout = Utility::ToBGFXLayout(bufferLayout);

        bgfx::VertexBufferHandle handle = createVertexBuffer(
            bgfx::makeRef(vertices.data(), vertices.size()),
            pvcLayout
        );

        if (!bgfx::isValid(handle)) {
            QS_CORE_ERROR("Failed to create VertexBuffer!");
            return {VertexBufferHandle::Invalid};
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

        if (!bgfx::isValid(handle)) [[unlikely]] {
            QS_CORE_ERROR("Failed to create IndexBuffer!");
            return {IndexBufferHandle::Invalid};
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

    TextureHandle bgfxRendererContext::CreateTexture(const TextureSpecification& spec) {
        bgfx::TextureHandle handle = TextureUtil::CreateTextureHandle(spec);
        return s_TextureDataTable.Emplace(handle, spec);
    }

    TextureHandle bgfxRendererContext::CreateTexture(const TextureSpecification& spec, Buffer data) {
        bgfx::TextureHandle handle = TextureUtil::CreateTextureHandle(spec, std::move(data));
        return s_TextureDataTable.Emplace(handle, spec);
    }

    TextureHandle bgfxRendererContext::CreateTexture(const TextureSpecification& spec,
                                                     const std::filesystem::path& path) {
        bgfx::TextureInfo info{};
        bimg::Orientation::Enum orientation;

        bgfx::TextureHandle handle = TextureUtil::LoadTexture(
            path,
            TextureUtil::ToBgfxTextureFlags(spec),
            &info,
            &orientation
        );

        return s_TextureDataTable.Emplace(handle, spec);
    }

    bool bgfxRendererContext::TextureIsVFlipped() {
        return bgfx::getCaps()->originBottomLeft;
    }

    const TextureSpecification* bgfxRendererContext::GetSpecification(const TextureHandle textureHandle) {
        const TextureData* textureData = s_TextureDataTable.Get(textureHandle);
        if (!textureData) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::GetSpecification",
                "Invalid texture handle ({},{})",
                textureHandle.GetIndex(), textureHandle.GetGeneration()
            );

            return nullptr;
        }

        return &textureData->Specification;
    }

    void bgfxRendererContext::TextureResize(const TextureHandle textureHandle, const uint32_t width,
                                            const uint32_t height) {
        TextureData* textureData = s_TextureDataTable.Get(textureHandle);
        if (!textureData) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::TextureResize",
                "Invalid texture handle ({},{})",
                textureHandle.GetIndex(), textureHandle.GetGeneration()
            );

            return;
        }

        TextureUtil::Resize(textureData, width, height);
    }

    uint16_t bgfxRendererContext::TextureGetNativeHandle(const TextureHandle textureHandle) {
        const auto* handle = s_TextureDataTable.Get(textureHandle);
        if (!handle) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::TextureGetNativeHandle",
                "Invalid texture handle ({},{})",
                textureHandle.GetIndex(),
                textureHandle.GetGeneration()
            );

            return 0;
        }

        return handle->Handle.idx;
    }

    void bgfxRendererContext::Bind(TextureHandle textureHandle) {
        //TODO: bgfx::setTexture(0, s_TextureDataTable.Get(textureHandle)->Handle);
    }

    void bgfxRendererContext::Destroy(const TextureHandle textureHandle) {
        bgfx::destroy(s_TextureDataTable.Get(textureHandle)->Handle);
        s_TextureDataTable.Erase(textureHandle);
    }

    FrameBufferHandle bgfxRendererContext::CreateFrameBuffer(uint32_t viewID, const Span<TextureHandle> attachments) {
        if (attachments.empty()) {
            QS_CORE_ERROR("bgfxRendererContext::CreateFrameBuffer", "Creating a FrameBuffer with no attachments!");
            return {FrameBufferHandle::Invalid};
        }

        FrameBufferData frameBufferData;
        frameBufferData.ViewId = viewID;

        for (auto attachment : attachments) {
            frameBufferData.Attachments.push_back(attachment);
        }

        SmallVec<bgfx::TextureHandle, 4> bgfxAttachments;
        bgfxAttachments.reserve(attachments.size());

        for (const auto& attachment : attachments) {
            const TextureData* textureData = s_TextureDataTable.Get(attachment);

            if (!textureData) {
                QS_CORE_ERROR_TAG(
                    "bgfxRendererContext::CreateFrameBufferHandle",
                    "Invalid texture handle while trying to create frame buffer!"
                );

                return {FrameBufferHandle::Invalid};
            }

            frameBufferData.Width = textureData->Specification.Width;
            frameBufferData.Height = textureData->Specification.Height;

            bgfxAttachments.push_back(textureData->Handle);
        }

        frameBufferData.Handle = bgfx::createFrameBuffer(
            static_cast<uint8_t>(bgfxAttachments.size()),
            bgfxAttachments.data(),
            true
        );

        if (!bgfx::isValid(frameBufferData.Handle)) {
            return {FrameBufferHandle::Invalid};
        }

        return s_FrameBufferTable.Emplace(frameBufferData);
    }

    uint32_t bgfxRendererContext::FrameBufferGetWidth(const FrameBufferHandle frameBufferHandle) {
        const auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            return 0;
        }

        return data->Width;
    }

    uint32_t bgfxRendererContext::FrameBufferGetHeight(const FrameBufferHandle frameBufferHandle) {
        const auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            return 0;
        }

        return data->Height;
    }

    glm::uvec2 bgfxRendererContext::FrameBufferGetSize(const FrameBufferHandle frameBufferHandle) {
        auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::FrameBufferGetSize",
                "Invalid frame buffer handle '({},{})'",
                frameBufferHandle.GetIndex(), frameBufferHandle.GetGeneration()
            );

            return glm::zero<glm::uvec2>();
        }

        return {data->Width, data->Height};
    }

    void bgfxRendererContext::FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId) {
        auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::FrameBufferSetViewID",
                "Invalid frame buffer handle '({},{})'",
                frameBufferHandle.GetIndex(), frameBufferHandle.GetGeneration()
            );

            return;
        }

        data->ViewId = viewId;
    }

    uint32_t bgfxRendererContext::FrameBufferGetViewID(const FrameBufferHandle frameBufferHandle) {
        const auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::FrameBufferGetViewID",
                "Invalid frame buffer handle '({},{})'",
                frameBufferHandle.GetIndex(), frameBufferHandle.GetGeneration()
            );

            return 0;
        }

        return data->ViewId;
    }

    void bgfxRendererContext::FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) {
        auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::FrameBufferResize",
                "Invalid frame buffer handle '({},{})'",
                frameBufferHandle.GetIndex(), frameBufferHandle.GetGeneration()
            );

            return;
        }

        if (width < 1 || height < 1 || (width == data->Width && height == data->Height)) {
            return;
        }

        data->Width = width;
        data->Height = height;

        if (bgfx::isValid(data->Handle)) {
            bgfx::destroy(data->Handle);
        }

        SmallVec<bgfx::TextureHandle, 4> attachments;
        for (const auto& attachment : data->Attachments) {
            auto* textureData = s_TextureDataTable.Get(attachment);
            TextureUtil::Resize(textureData, width, height);
            attachments.push_back(textureData->Handle);
        }

        data->Handle = bgfx::createFrameBuffer(
            static_cast<uint8_t>(attachments.size()),
            attachments.data(),
            false
        );

        data->Width = width;
        data->Height = height;
    }

    void bgfxRendererContext::Bind(const FrameBufferHandle frameBufferHandle) {
        const auto* data = s_FrameBufferTable.Get(frameBufferHandle);
        if (!data) {
            QS_CORE_ERROR_TAG(
                "bgfxRendererContext::FrameBufferGetSize",
                "Invalid frame buffer handle '({},{})'",
                frameBufferHandle.GetIndex(), frameBufferHandle.GetGeneration()
            );

            return;
        }

        bgfx::setViewFrameBuffer(data->ViewId, data->Handle);
    }

    void bgfxRendererContext::Destroy(const FrameBufferHandle frameBufferHandle) {
        bgfx::destroy(s_FrameBufferTable.Get(frameBufferHandle)->Handle);
        s_FrameBufferTable.Erase(frameBufferHandle);
    }

    bgfx::TextureHandle bgfxRendererContext::GetBgfxTextureHandle(const TextureHandle handle) {
        return s_TextureDataTable.Get(handle)->Handle;
    }
}
