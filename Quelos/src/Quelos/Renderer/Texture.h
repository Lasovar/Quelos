#pragma once

#include <filesystem>

#include "glm/vec2.hpp"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Core/Ref.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class QS_API ImageFormat {
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

    enum class QS_API TextureWrap {
        Clamp,
        Repeat
    };

    enum class QS_API TextureFilter {
        Linear,
        Nearest,
        Anisotropic,
    };

    enum class QS_API TextureType {
        Texture2D,
        TextureCube
    };

    enum class QS_API TextureRenderTarget {
        Off,
        ReadWrite,
        WriteOnly
    };

    enum class QS_API RenderTargetMSAA {
        None,
        MSAA_X2,
        MSAA_X4,
        MSAA_X8,
        MSAA_X16,
    };

    struct QS_API TextureSpecification {
        ImageFormat Format = ImageFormat::RGBA;
        uint32_t Width = 1;
        uint32_t Height = 1;
        TextureWrap SamplerWrap = TextureWrap::Repeat;
        TextureFilter SamplerFilter = TextureFilter::Linear;

        bool IsBlitDestination = false;
        TextureRenderTarget RenderTarget = TextureRenderTarget::Off;
        RenderTargetMSAA MSAAType = RenderTargetMSAA::None;
    };

    class Texture;

    struct TextureHandle : Handle<Texture> {
        TextureHandle() = default;
        TextureHandle(const Handle handle) : Handle(handle) {}

        [[nodiscard]] uint16_t GetNativeHandle() const;
    };

    class QS_API Texture : public Asset {
    public:
        ~Texture() override = default;

        virtual ImageFormat GetFormat() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual glm::uvec2 GetSize() const = 0;

        virtual bool IsVFlipped() const = 0;

        virtual TextureType GetType() const = 0;

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<Texture>();
            return s_AssetType;
        }

        const AssetType& GetAssetType() const override { return GetStaticType(); }

        virtual TextureHandle GetHandle() const = 0;
    private:
    };

    class QS_API Texture2D : public Texture {
    public:
        Texture2D(const TextureHandle& handle) : m_Handle(handle) {}
        ~Texture2D() override;

        static Ref<Texture2D> Create(const TextureSpecification& spec);
        static Ref<Texture2D> Create(const TextureSpecification& spec, Buffer data);
        static Ref<Texture2D> Create(const TextureSpecification& spec, const OsPath& texturePath);

    public:
        void Resize(const glm::uvec2& size) const;
        void Resize(uint32_t width, uint32_t height) const;

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<Texture2D>();
            return s_AssetType;
        }

        const AssetType& GetAssetType() const override { return GetStaticType(); }

        TextureType GetType() const override { return TextureType::Texture2D; }
        ImageFormat GetFormat() const override;
        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        glm::uvec2 GetSize() const override;
        bool IsVFlipped() const override;
        TextureHandle GetHandle() const override { return m_Handle; }

    private:
        TextureHandle m_Handle;
    };
}
