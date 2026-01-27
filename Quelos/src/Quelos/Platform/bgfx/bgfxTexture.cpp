#include "qspch.h"

#include "bgfxTexture.h"
#include <bx/file.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    static void imageReleaseCb(void* _ptr, void* _userData) {
        BX_UNUSED(_ptr);
        const auto imageContainer = static_cast<bimg::ImageContainer*>(_userData);
        bimg::imageFree(imageContainer);
    }

    bgfx::TextureHandle LoadTexture(const std::filesystem::path& filePath, const uint64_t flags,
        bgfx::TextureInfo* info, bimg::Orientation::Enum* orientation) {
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        bx::DefaultAllocator allocator;
        if (const std::vector<byte> data = Utility::ReadBinaryFile(filePath); !data.empty()) {
            bimg::ImageContainer* imageContainer = bimg::imageParse(&allocator, data.data(), data.size());
            if (imageContainer != nullptr) {
                if (orientation != nullptr) {
                    *orientation = imageContainer->m_orientation;
                }

                const bgfx::Memory* mem = bgfx::makeRef(
                    imageContainer->m_data,
                    imageContainer->m_size,
                    imageReleaseCb,
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

    bgfxTexture2D::bgfxTexture2D(const TextureSpecification& spec, const std::filesystem::path& path) {
        m_Specification = spec;
        m_Path = path;

        bgfx::TextureInfo info{};
        bimg::Orientation::Enum orientation;

        m_Handle = LoadTexture(
            path,
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
            &info,
            &orientation
        );
    }

    void bgfxTexture2D::Bind(uint32_t slot) const {
    }

    ImageFormat bgfxTexture2D::GetFormat() const { return ImageFormat::RGBA; }

    void bgfxTexture2D::CreateFromFile(const TextureSpecification& specification,
        const std::filesystem::path& filepath) {
    }

    void bgfxTexture2D::Resize(const glm::uvec2& size) {
    }

    void bgfxTexture2D::Resize(uint32_t width, uint32_t height) {
    }
}
