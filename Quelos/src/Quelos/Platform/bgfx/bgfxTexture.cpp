#include "qspch.h"

#include "bgfxTexture.h"
#include <bx/file.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    static void ImageReleaseCb(void* pointer, void* userData) {
        BX_UNUSED(pointer);
        const auto imageContainer = static_cast<bimg::ImageContainer*>(userData);
        bimg::imageFree(imageContainer);
    }

    static bgfx::TextureHandle LoadTexture(const std::filesystem::path& filePath, const uint64_t flags,
        bgfx::TextureInfo* info, bimg::Orientation::Enum* orientation) {
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        bx::DefaultAllocator allocator;
        if (const std::vector<byte> data = Utility::ReadBinaryFile(filePath); !data.empty()) {
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
                                              static_cast<bgfx::TextureFormat::Enum>(imageContainer->m_format), flags)) {
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
            case ImageFormat::RGBA:  return bgfx::TextureFormat::RGBA8;
            case ImageFormat::RGB:   return bgfx::TextureFormat::RGB8;
            case ImageFormat::RED8UN:   return bgfx::TextureFormat::R8;
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

    static uint64_t ToBgfxTextureFlags(const TextureSpecification& spec) {
        uint64_t flags = ToBgfxSamplerFlags(spec.SamplerWrap) | ToBgfxSamplerFlags(spec.SamplerFilter);
        if (spec.IsRenderTarget) {
            flags |= BGFX_TEXTURE_RT;
        }
        return flags;
    }

    static bgfx::TextureHandle CreateTextureHandle(const TextureSpecification& spec) {
        return bgfx::createTexture2D(
            static_cast<uint16_t>(spec.Width),
            static_cast<uint16_t>(spec.Height),
            false,
            1,
            QuelosImageFormatToBgfxFormat(spec.Format),
            ToBgfxTextureFlags(spec)
        );
    }

    bgfxTexture2D::bgfxTexture2D(const TextureSpecification& spec) {
        m_Specification = spec;

        m_Handle = CreateTextureHandle(spec);
    }

    bgfxTexture2D::bgfxTexture2D(const TextureSpecification& spec, const std::filesystem::path& path) {
        m_Specification = spec;
        m_Path = path;

        bgfx::TextureInfo info{};
        bimg::Orientation::Enum orientation;

        m_Handle = LoadTexture(
            path,
            ToBgfxTextureFlags(spec),
            &info,
            &orientation
        );
    }

    ImageFormat bgfxTexture2D::GetFormat() const { return m_Specification.Format; }

    void bgfxTexture2D::CreateFromFile(const TextureSpecification& specification,
        const std::filesystem::path& filepath) {
    }

    bool bgfxTexture2D::IsVFlipped() const {
        return bgfx::getCaps()->originBottomLeft;
    }

    void bgfxTexture2D::Resize(const glm::uvec2& size) {
        Resize(size.x, size.y);
    }

    void bgfxTexture2D::Resize(const uint32_t width, const uint32_t height) {
        if (bgfx::isValid(m_Handle)) {
            bgfx::destroy(m_Handle);
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        m_Handle = CreateTextureHandle(m_Specification);
    }

    bgfxTexture2D::~bgfxTexture2D() {
        if (bgfx::isValid(m_Handle)) {
            bgfx::destroy(m_Handle);
        }
    }
}
