#pragma once

#include <filesystem>

#include "GraphicsTypes.h"
#include "PipelineResourceSignature.h"
#include "Quelos/Math/Math.h"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Core/Ref.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class QS_API ImageFormat {
        None = 0,
        R8UNorm,
        R8UInt,
        R16UInt,
        R32UInt,
        R32Float,
        RG8UNorm,
        RG16Float,
        RG32Float,
        RGB,
        RGBA8UNorm,
        RGBA8SNorm,
        RGBA16Float,
        RGBA32Float,

        B10R11G11Float,

        SRGB,
        SRGBA,

        Depth32FloatStencil8UInt,
        Depth32Float,
        Depth24Stencil8,

        // Defaults
        Depth = Depth24Stencil8,
    };

    enum class QS_API TextureType {
        Texture2D,
        Texture2DArray,
        TextureCube
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

        ImageFormat Format = ImageFormat::RGBA8UNorm;
        uint32_t Width = 1;
        uint32_t Height = 1;
        WrapMode SamplerWrap = WrapMode::Repeat;
        FilterMode SamplerFilter = FilterMode::Linear;
        TextureType Type = TextureType::Texture2D;
        Usage Usage = Usage::Default;

        bool IsBlitDestination = false;
        BindFlags BindFlags = Bind::None;
        SampleCount SampleCount = SampleCount::x1;
        CpuAccessFlags CpuAccessFlags = CpuAccess::None;

        /// For a 1D array or 2D array, number of array slices.
        ///
        /// For cube maps and cube map arrays, this value must be a multiple of 6.
        /// The number of cube maps in the texture is ArraySize / 6.
        uint32_t ArraySize = 1;

        /// Number of Mip levels in the texture. Multisampled textures can only have 1 Mip level.

        /// Specify 0 to create full mipmap chain.
        uint32_t MipLevels = 1;
    };

    class Texture;

    struct QS_API TextureHandle : Handle<Texture> {
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
        Texture2D(const TextureSpecification& spec, BufferView data);
        Texture2D(const TextureHandle& handle) : m_Handle(handle) {}
        ~Texture2D() override;

        static Ref<Texture2D> Create(const TextureSpecification& spec);
        static Ref<Texture2D> Create(const TextureSpecification& spec, BufferView data);
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

    enum class QS_API TextureViewType : uint8_t {
        /// Undefined view type
        Undefined = 0,

        /// A texture view will define a shader resource view that will be used
        /// as the source for the shader read operations
        ShaderResource,

        /// A texture view will define a render target view that will be used
        /// as the target for rendering operations
        RenderTarget,

        /// A texture view will define a depth stencil view that will be used
        /// as the target for rendering operations
        DepthStencil,

        /// A texture view will define a read-only depth stencil view that will be used
        /// as depth stencil source for rendering operations, but can also be simultaneously
        /// read from shaders.
        ReadOnlyDepthStencil,

        /// A texture view will define an unordered access view that will be used
        /// for unordered read/write operations from the shaders
        UnorderedAccess,

        /// A texture view will define a variable shading rate view that will be used
        /// as the shading rate source for rendering operations
        ShadingRate,

        /// Helper value that stores that total number of texture views
        Count
    };

    enum class UAVAccessFlags : uint8_t {
        /// Access mode is unspecified
        Unspecified = 0x00,

        /// Allow read operations on the UAV
        Read   = 0x01,

        /// Allow write operations on the UAV
        Write  = 0x02,

        /// Allow read and write operations on the UAV
        ReadWrite = Read | Write,

        Last = ReadWrite
    };

    enum class TextureViewFlags : uint8_t {
        /// No flags
        None = 0,

        /// Allow automatic mipmap generation for this view.
        /// This flag is only allowed for TEXTURE_VIEW_SHADER_RESOURCE view type.
        /// The texture must be created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag.
        AllowMipMapGeneration = 1u << 0,

        TEXTURE_VIEW_FLAG_LAST = AllowMipMapGeneration
    };

    struct TextureViewSpec {
        TextureViewType ViewType = TextureViewType::Undefined;
        TextureType TextureType = TextureType::Texture2D;

        /// View format.

        /// If default value ImageFormat::None is provided,
        /// the view format will match the referenced texture format.
        ImageFormat Format = ImageFormat::None;

        union {
            /// For a texture array, first array slice to address in the view
            uint32_t FirstArraySlice = 0;

            /// For a 3D texture, first depth slice to address the view
            uint32_t FirstDepthSlice;
        };

        union {
            /// For a texture array, number of array slices to address in the view.

            /// Set to 0 to address all array slices.
            uint32_t NumArraySlices = 0;


            /// For a 3D texture, number of depth slices to address in the view

            /// Set to 0 to address all depth slices.
            uint32_t NumDepthSlices;
        };

        uint32_t MostDetailedMip = 0;
        uint32_t MipLevels = 1;
        uint32_t NumMipLevels = 0;

        UAVAccessFlags AccessFlags = UAVAccessFlags::Unspecified;

        TextureViewFlags Flags = TextureViewFlags::None;
    };

    class TextureView;

    struct TextureViewHandle : Handle<TextureView> {
        TextureViewHandle() = default;
        TextureViewHandle(const Handle handle) : Handle(handle) {}
    };


    struct MappedTextureSubresource {
        /// Pointer to the mapped subresource data in CPU memory.
        void* Data = nullptr;

        /// Row stride in bytes.
        uint64_t Stride = 0;

        /// Depth slice stride in bytes.
        uint64_t DepthStride = 0;
    };
}
