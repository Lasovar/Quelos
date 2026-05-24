#pragma once

#include <filesystem>

#include "Quelos/Math/Math.h"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Core/Ref.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class QS_API ImageFormat {
        None = 0,
        RED8UNORM,
        RED8UINT,
        RED16UINT,
        RED32UINT,
        RED32FLOAT,
        RG8UNORM,
        RG16FLOAT,
        RG32FLOAT,
        RGB,
        RGBA,
        RGBA16FLOAT,
        RGBA32FLOAT,

        B10R11G11FLOAT,

        SRGB,
        SRGBA,

        DEPTH32FSTENCIL8UINT,
        DEPTH32F,
        DEPTH24STENCIL8,

        // Defaults
        Depth = DEPTH24STENCIL8,
    };

    enum class QS_API TextureWrap {
        Unknown,
        Repeat,
        Clamp,
        Mirror,
        Border,

        /// Similar to TEXTURE_ADDRESS_MIRROR and TEXTURE_ADDRESS_CLAMP. Takes the absolute
        /// value of the texture coordinate (thus, mirroring around 0), and then clamps to
        /// the maximum value. \n
        /// Direct3D Counterpart: D3D11_TEXTURE_ADDRESS_MIRROR_ONCE/D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE. OpenGL counterpart: GL_MIRROR_CLAMP_TO_EDGE
        /// \note GL_MIRROR_CLAMP_TO_EDGE is only available in OpenGL4.4+, and is not available until at least OpenGLES3.1
        MirrorOnce,
        Count
    };

    enum class QS_API TextureFilter {
        Unknown,
        Point,
        Linear,
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
        std::string Name;

        ImageFormat Format = ImageFormat::RGBA;
        uint32_t Width = 1;
        uint32_t Height = 1;
        TextureWrap SamplerWrap = TextureWrap::Repeat;
        TextureFilter SamplerFilter = TextureFilter::Linear;
        TextureType Type = TextureType::Texture2D;

        bool IsBlitDestination = false;
        TextureRenderTarget RenderTarget = TextureRenderTarget::Off;
        RenderTargetMSAA MSAAType = RenderTargetMSAA::None;
    };

    class Texture;

    struct TextureHandle : Handle<Texture> {
        TextureHandle() = default;
        TextureHandle(const Handle handle) : Handle(handle) {}

        [[nodiscard]] uint64_t GetNativeHandle() const;
    };

    class QS_API Texture : public Asset {
    public:
        ~Texture() override = default;

        virtual ImageFormat GetFormat() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint2 GetSize() const = 0;

        virtual bool IsVFlipped() const = 0;

        virtual TextureType GetType() const = 0;

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<Texture>();
            return s_AssetType;
        }

        const AssetType& GetAssetType() const override { return GetStaticType(); }

        virtual TextureHandle GetHandle() const = 0;
    };

    class QS_API Texture2D : public Texture {
    public:
        Texture2D(const TextureSpecification& spec, Buffer data);
        Texture2D(const TextureHandle& handle) : m_Handle(handle) {}
        ~Texture2D() override;

        static Ref<Texture2D> Create(const TextureSpecification& spec);
        static Ref<Texture2D> Create(const TextureSpecification& spec, Buffer data);
        static Ref<Texture2D> Create(const TextureSpecification& spec, const OsPath& texturePath);

    public:
        void Resize(const uint2& size) const;
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
        uint2 GetSize() const override;
        bool IsVFlipped() const override;
        TextureHandle GetHandle() const override { return m_Handle; }

    private:
        TextureHandle m_Handle;
    };
}
