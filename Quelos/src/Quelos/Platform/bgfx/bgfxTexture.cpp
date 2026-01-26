#include "qspch.h"

#include "bgfxTexture.h"
#include <bx/file.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

namespace Quelos {
    void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const bx::FilePath& _filePath, uint32_t* _size) {
        if (bx::open(_reader, _filePath)) {
            uint32_t size = (uint32_t)bx::getSize(_reader);
            void* data = bx::alloc(_allocator, size);
            bx::read(_reader, data, size, bx::ErrorAssert{});
            bx::close(_reader);
            if (NULL != _size) {
                *_size = size;
            }
            return data;
        }
        else {
            DBG("Failed to open: %s.", _filePath.getCPtr());
        }

        if (NULL != _size) {
            *_size = 0;
        }

        return NULL;
    }

    void unload(void* _ptr, bx::AllocatorI* _allocator) {
        bx::free(_allocator, _ptr);
    }

    static void imageReleaseCb(void* _ptr, void* _userData) {
        BX_UNUSED(_ptr);
        bimg::ImageContainer* imageContainer = static_cast<bimg::ImageContainer*>(_userData);
        bimg::imageFree(imageContainer);
    }

    bgfx::TextureHandle loadTexture(bx::FileReaderI* _reader, const bx::FilePath& _filePath, uint64_t _flags,
                                    uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation) {
        BX_UNUSED(_skip);
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        bx::DefaultAllocator allocator;
        uint32_t size;
        void* data = load(_reader, &allocator, _filePath, &size);
        if (data != nullptr) {
            bimg::ImageContainer imageContainer;
            if (bimg::imageParse(imageContainer, data, size)) {
                if (_orientation != nullptr) {
                    *_orientation = imageContainer.m_orientation;
                }

                const bgfx::Memory* mem = bgfx::makeRef(
                    imageContainer.m_data
                    , imageContainer.m_size
                    , imageReleaseCb
                    , &imageContainer
                );

                unload(data, &allocator);

                if (_info != nullptr) {
                    bgfx::calcTextureSize(
                        *_info,
                        static_cast<uint16_t>(imageContainer.m_width),
                        static_cast<uint16_t>(imageContainer.m_height),
                        static_cast<uint16_t>(imageContainer.m_depth),
                        imageContainer.m_cubeMap,
                        1 < imageContainer.m_numMips,
                        imageContainer.m_numLayers,
                        static_cast<bgfx::TextureFormat::Enum>(imageContainer.m_format)
                    );
                }

                if (imageContainer.m_cubeMap) {
                    handle = bgfx::createTextureCube(
                        static_cast<uint16_t>(imageContainer.m_width),
                        1 < imageContainer.m_numMips,
                        imageContainer.m_numLayers,
                        static_cast<bgfx::TextureFormat::Enum>(imageContainer.m_format),
                        _flags,
                        mem
                    );
                }
                else if (1 < imageContainer.m_depth) {
                    handle = bgfx::createTexture3D(
                        static_cast<uint16_t>(imageContainer.m_width),
                        static_cast<uint16_t>(imageContainer.m_height),
                        static_cast<uint16_t>(imageContainer.m_depth),
                        1 < imageContainer.m_numMips,
                        static_cast<bgfx::TextureFormat::Enum>(imageContainer.m_format),
                        _flags,
                        mem
                    );
                }
                else if (bgfx::isTextureValid(0, false, imageContainer.m_numLayers,
                                              static_cast<bgfx::TextureFormat::Enum>(imageContainer.m_format), _flags)) {
                    handle = bgfx::createTexture2D(
                        static_cast<uint16_t>(imageContainer.m_width),
                        static_cast<uint16_t>(imageContainer.m_height),
                        1 < imageContainer.m_numMips,
                        imageContainer.m_numLayers,
                        static_cast<bgfx::TextureFormat::Enum>(imageContainer.m_format),
                        _flags,
                        mem
                    );
                }

                if (bgfx::isValid(handle)) {
                    const bx::StringView name(_filePath);
                    bgfx::setName(handle, name.getPtr(), name.getLength());
                }
            }
        }

        return handle;
    }

    bgfxTexture2D::bgfxTexture2D(const TextureSpecification& spec, const std::filesystem::path& path) {
    }
}
