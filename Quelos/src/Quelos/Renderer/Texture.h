#pragma once
#include <filesystem>

#include "glm/vec2.hpp"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Core/Ref.h"

namespace Quelos {

    enum class ImageFormat {
        None = 0,
        RED8UN,
        RED8UI,
        RED16UI,
        RED32UI,
        RED32F,
        RG8,
        RG16F,
        RG32F,
        RGB,
        RGBA,
        RGBA16F,
        RGBA32F,

        B10R11G11UF,

        SRGB,
        SRGBA,

        DEPTH32FSTENCIL8UINT,
        DEPTH32F,
        DEPTH24STENCIL8,

        // Defaults
        Depth = DEPTH24STENCIL8,
    };

    enum class TextureWrap {
        None = 0,
        Clamp,
        Repeat
    };

    enum class TextureFilter {
        None = 0,
        Linear,
        Nearest,
        Cubic
    };

    enum class TextureType {
        None = 0,
        Texture2D,
        TextureCube
    };

    struct TextureSpecification {
        ImageFormat Format = ImageFormat::RGBA;
        uint32_t Width = 1;
        uint32_t Height = 1;
        TextureWrap SamplerWrap = TextureWrap::Repeat;
        TextureFilter SamplerFilter = TextureFilter::Linear;
    };

    class Texture : Asset {
        virtual ImageFormat GetFormat() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual glm::uvec2 GetSize() const = 0;

        virtual TextureType GetType() const = 0;

        static AssetType GetStaticType() { return AssetType::Texture; }
        AssetType GetAssetType() const override { return GetStaticType(); }

        virtual void* GetNativeHandle() const = 0;
    };

    class Texture2D : public Texture {
    public:
        static Ref<Texture2D> Create(const TextureSpecification& spec);
        static Ref<Texture2D> Create(const std::filesystem::path& texturePath);

        virtual void CreateFromFile(const TextureSpecification& specification, const std::filesystem::path& filepath) = 0;

        virtual void Resize(const glm::uvec2& size) = 0;
        virtual void Resize(const uint32_t width, const uint32_t height) = 0;

		virtual const std::filesystem::path& GetPath() const = 0;

        TextureType GetType() const override { return TextureType::Texture2D; }
    };
}
