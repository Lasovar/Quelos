#include "qspch.h"

#include "stb_image.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Project/Project.h"

#include "TextureImporter.h"

#include "Quelos/Renderer/Renderer.h"

namespace QuelosEditor {
    static float1 SRGBToLinear(const float1& c) {
        return math::all(c <= float1(0.04045f)) ? c / 12.92f : math::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    static float1 LinearToSRGB(const float1& c) {
        return math::all(c <= float1(0.0031308f)) ? c * 12.92f : 1.055f * math::pow(c, 1.0f / 2.4f) - 0.055f;
    }

    static byte ToUNorm8(const float& v) {
        return static_cast<byte>(
            math::lround(math::clamp(v, 0.0f, 1.0f) * float1(255.0f))
        );
    }

    static void PadTo(Vec64<byte>& buf, const uint64_t target) {
        QS_ASSERT(buf.size() <= target, "Cooked data overran its precomputed offset!");
        buf.resize(target);
    }

    namespace Cooker {
        namespace {
            struct ImageF {
                uint32_t Width = 0, Height = 0;
                Vec64<float4> Texels;   // RGBA, linear [0,1]
            };

            struct MipLevel {
                uint32_t Width = 0, Height = 0;
                Vec64<float4> Texels;

                [[nodiscard]] float4& At(const uint32_t x, const uint32_t y) {
                    return Texels[static_cast<size_t>(y) * Width + x];
                }

                [[nodiscard]] const float4& At(const uint32_t x, const uint32_t y) const {
                    return Texels[static_cast<size_t>(y) * Width + x];
                }
            };
        }

        using MipChain = Vec<MipLevel>;

        // 2x2 reduce; clamps at the edge so odd sizes work
        template <typename Combine>
        static MipLevel Downsample(const MipLevel& src, Combine&& combine, std::pmr::memory_resource* memoryResource) {
            MipLevel dst {
                .Width = std::max(src.Width / 2, 1u),
                .Height = std::max(src.Height / 2, 1u),
                .Texels = Vec64<float4>(memoryResource)
            };

            dst.Texels.resize(static_cast<size_t>(dst.Width) * dst.Height);
            for (uint32_t y = 0; y < dst.Height; y++) {
                for (uint32_t x = 0; x < dst.Width; x++) {
                    const uint32_t x0 = std::min(2 * x, src.Width - 1);
                    const uint32_t x1 = std::min(2 * x + 1, src.Width - 1);
                    const uint32_t y0 = std::min(2 * y, src.Height - 1);
                    const uint32_t y1 = std::min(2 * y + 1, src.Height - 1);

                    dst.At(x, y) = combine(src.At(x0, y0), src.At(x1, y0), src.At(x0, y1), src.At(x1, y1));
                }
            }

            return dst;
        }

        template <typename Combine>
        static void BuildMips(MipChain& chain, Combine&& combine, std::pmr::memory_resource* memoryResource) {
            while (chain.back().Width > 1 || chain.back().Height > 1) {
                chain.push_back(Downsample(chain.back(), combine, memoryResource));
            }
        }

        static constexpr uint64_t size(const uint32_t width, const uint32_t height, const uint32_t channels) {
            return static_cast<uint64_t>(width) * height * channels;
        }

        static void Quantize(const MipLevel& mip, const uint32_t channels, Vec64<byte>& out) {
            // 4 = RGBA8, 2 = RG8
            const uint64_t base = out.size();
            out.resize(base + size(mip.Width, mip.Height, channels));
            for (size_t i = 0; i < mip.Texels.size(); ++i) {
                const float4& t = mip.Texels[i];
                for (uint32_t c = 0; c < channels; ++c) {
                    out[base + i * channels + c] = ToUNorm8(t[static_cast<int>(c)]);
                }
            }
        }

        static MipChain CookColor(ImageF&& img, const bool srgb, std::pmr::memory_resource* memoryResource) {
            MipChain chain(memoryResource);
            chain.emplace_back(img.Width, img.Height, std::move(img.Texels));

            // filter in linear space; alpha is never sRGB
            if (srgb) {
                for (float4& t : chain[0].Texels) {
                    t = float4(SRGBToLinear(t.x), SRGBToLinear(t.y), SRGBToLinear(t.z), t.w);
                }
            }

            BuildMips(chain, [](const float4& a, const float4& b, const float4& c, const float4& d) {
                return (a + b + c + d) * 0.25f;
            }, memoryResource);

            // back to sRGB for storage
            if (srgb) {
                for (MipLevel& mip : chain) {
                    for (float4& t : mip.Texels) {
                        t = float4(LinearToSRGB(t.x), LinearToSRGB(t.y), LinearToSRGB(t.z), t.w);
                    }
                }
            }

            return chain;   // RGBA8_UNORM_SRGB now, BC7_UNORM_SRGB later (RGBA8_UNORM / BC7_UNORM if !srgb)
        }

        static MipChain CookNormal(ImageF&& img, bool flipGreen, std::pmr::memory_resource* memoryResource) {
            MipChain chain(memoryResource);
            chain.emplace_back(img.Width, img.Height, std::move(img.Texels));

            for (float4& t : chain[0].Texels) {
                float3 n = t.xyz * 2.0f - 1.0f;
                if (flipGreen) {
                    n.y = -n.y;
                }

                n.z = math::max(n.z, float1(0.0f));
                t = float4(math::normalize(n), 1.0f);
            }

            BuildMips(chain, [](const float4& a, const float4& b, const float4& c, const float4& d) {
                const float3 avg = (a.xyz + b.xyz + c.xyz + d.xyz) * 0.25f;
                const float1 len = math::length(avg);
                return float4(avg / math::max(len, float1(1e-6f)), len);    // w = length, kept for Toksvig later
            }, memoryResource);

            for (MipLevel& mip : chain) {
                for (float4& t : mip.Texels) {
                    t = float4(t.xy * 0.5f + 0.5f, 0.0f, 1.0f);    // only RG is stored
                }
            }

            return chain;   // RG8_UNORM now, BC5_UNORM later
        }
    }

    bool TextureImporter::IsAssetSupported(const std::string_view path) {
        static HashSet<std::string_view> s_ImportableExtensions = {
            {
                ".png", ".jpeg"
            },
            AllocatorType::Persistent
        };

        return s_ImportableExtensions.find(FS::Extension(path)) != s_ImportableExtensions.end();
    }

    constexpr uint32_t k_TextureMagic   = 'Q' | ('T' << 8) | ('E' << 16) | ('X' << 24);
    constexpr uint32_t k_TextureVersion = 1;

    namespace {
        struct CookedTextureHeader {
            uint32_t Magic = k_TextureMagic;
            uint32_t Version = k_TextureVersion;
            uint32_t Format = 0;       // your TextureFormat enum
            uint32_t Width = 0, Height = 0;
            uint32_t MipLevels = 1;
            uint32_t ArraySize = 1;  // 6 for cubes later
            uint32_t Flags = 0;        // IsCube, etc.
        };

        struct CookedMip {
            uint64_t Offset = 0;       // from file start
            uint64_t Size = 0;
            uint32_t Width = 0, Height = 0;
            uint32_t RowPitch = 0;     // w * bytesPerPixel; for BC: ceil(w/4) * blockBytes
            uint32_t Pad = 0;
        };
    }


    bool TextureImporter::ImportTexture2D(LoadContext& loadContext) {
        const auto blob = loadContext.ReadArtifact("TextureData");
        if (!blob) {
            return false;
        }

        Serialization::BinaryReader reader(*blob);
        auto header = reader.Read<CookedTextureHeader>();
        if (!header || header->Magic != k_TextureMagic || header->Version != k_TextureVersion) {
            return false;
        }

        Vec<CookedMip> mips(header->MipLevels, Allocator::Frame);
        for (CookedMip& mip : mips) {
            auto mipData = reader.Read<CookedMip>();
            if (!mipData) {
                return false;
            }

            mip = *mipData;
        }

        Vec<TextureSubResourceData> subRes(header->MipLevels, Allocator::Frame);
        for (uint32_t i = 0; i < header->MipLevels; ++i) {
            subRes[i].pData  = blob->data() + mips[i].Offset;
            subRes[i].Stride = mips[i].RowPitch;
        }

        TextureSpec textureSpecs;
        textureSpecs.Name = FS::Filename(loadContext.GetAssetMetadata().FilePath);
        textureSpecs.Width = header->Width;
        textureSpecs.Height = header->Height;
        textureSpecs.Format = static_cast<TextureFormat>(header->Format);
        textureSpecs.ArraySize = header->ArraySize;
        textureSpecs.MipLevels = header->MipLevels;
        textureSpecs.BindFlags = Bind::ShaderResource;

        if (const Optional<TextureSettings> settings = loadContext.Settings<TextureSettings>()) {
            textureSpecs.SamplerFilter = settings->Filter;
            textureSpecs.SamplerWrap = settings->Wrap;
        }

        auto* texture = new(loadContext.GetLoadSlot()) Texture(textureSpecs, subRes);
        texture->SetAssetID(loadContext.GetAssetMetadata().Handle);

        return true;
    }

    static Optional<Cooker::ImageF> LoadImageF(const char* filePath, std::pmr::memory_resource* memoryResource) {
        int w, h, channels;
        Cooker::ImageF result{ .Texels = Vec64<float4>(memoryResource) };

        if (stbi_is_16_bit(filePath)) {
            stbi_us* px = stbi_load_16(filePath, &w, &h, &channels, 4);
            if (!px) {
                return None;
            }

            result.Texels.resize(static_cast<size_t>(w) * h);
            for (size_t i = 0; i < result.Texels.size(); ++i) {
                result.Texels[i] = float4(px[i*4+0], px[i*4+1], px[i*4+2], px[i*4+3]) / 65535.0f;
            }

            stbi_image_free(px);
        } else {
            stbi_uc* px = stbi_load(filePath, &w, &h, &channels, 4);
            if (!px) {
                return None;
            }

            result.Texels.resize(static_cast<size_t>(w) * h);
            for (size_t i = 0; i < result.Texels.size(); ++i) {
                result.Texels[i] = float4(px[i*4+0], px[i*4+1], px[i*4+2], px[i*4+3]) / 255.0f;
            }

            stbi_image_free(px);
        }

        result.Width = w;
        result.Height = h;

        return result;
    }

    bool TextureImporter::Import(ImportContext& importContext) {
        const auto& metadata = importContext.GetAssetMetadata();

        const OsPath assetPath = Project::GetProjectPath() / metadata.FilePath;
        stbi_set_flip_vertically_on_load(1);
        Optional<Cooker::ImageF> result = LoadImageF(assetPath.string().c_str(), &GetFrameAllocator());

        if (!result) {
            QS_CORE_ERROR_TAG(
                "TextureImporter::ImportTexture2D", "Failed to import texture '{}': {}",
                assetPath.string(),
                stbi_failure_reason()
            );

            return false;
        }

        auto settings = importContext.Settings<TextureSettings>();
        if (!settings) {
            return false;
        }

        Cooker::MipChain chain;
        TextureFormat format = TextureFormat::SRGBA;
        switch (settings->Usage) {
        case TextureSettings::Usage::Default:
            chain = Cooker::CookColor(std::move(*result), settings->IsSRGB, &GetFrameAllocator());
            format = settings->IsSRGB ? TextureFormat::SRGBA : TextureFormat::RGBA8UNorm;
            break;
        case TextureSettings::Usage::Normal:
            chain = Cooker::CookNormal(std::move(*result), settings->FlipGreen, &GetFrameAllocator());
            format = TextureFormat::RG8UNorm;
            break;
        default:
            return false;
        }

        const CookedTextureHeader header {
            .Format = static_cast<uint32_t>(format),
            .Width = chain[0].Width,
            .Height = chain[0].Height,
            .MipLevels = chain.size(),
            .ArraySize = 1,
        };

        const uint32_t channels = format == TextureFormat::RG8UNorm ? 2 : 4;
        const uint32_t mipLevels = chain.size();

        uint64_t offset = AlignUp(sizeof(CookedTextureHeader) + sizeof(CookedMip) * mipLevels, 16);
        Vec<CookedMip> mips(&GetFrameAllocator());
        for (const Cooker::MipLevel& mipLevel : chain) {
            const uint64_t size = Cooker::size(mipLevel.Width, mipLevel.Height, channels);
            mips.push_back({
                .Offset = offset,
                .Size = Cooker::size(mipLevel.Width, mipLevel.Height, channels),
                .Width = mipLevel.Width,
                .Height = mipLevel.Height,
                .RowPitch = mipLevel.Width * channels,
            });

            offset = AlignUp(offset + size, 16);
        }

        Vec64<byte> buffer(Allocator::Persistent);
        Serialization::BinaryWriter writer(buffer);

        writer.Write(header);
        for (const CookedMip& mip : mips) {
            writer.Write(mip);
        }

        for (uint32_t i = 0; i < mipLevels; ++i) {
            PadTo(buffer, mips[i].Offset);
            Cooker::Quantize(chain[i], channels, buffer);
        }

        if (auto r = importContext.WriteArtifact("TextureData", buffer); !r) {
            QS_ERROR_TAGGED("Failed to write texture data artifact: {}", r.error());
            return false;
        }

        return true;
    }
}
