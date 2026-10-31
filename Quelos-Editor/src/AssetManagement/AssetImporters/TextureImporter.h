#pragma once

#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetMetadata.h"
#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/Renderer/Texture.h"

namespace QuelosEditor {
    struct TextureSettings {
        enum class Usage {
            Default,
            Normal,
            Environment
        };

        enum class CompressionQuality {
            None,
            Low,
            Normal,
            High
        };

        Usage Usage = Usage::Default;
        FilterMode Filter = FilterMode::Linear;
        WrapMode Wrap = WrapMode::Repeat;
        bool IsSRGB = true;
        bool FlipGreen = false;
        CompressionQuality Compression = CompressionQuality::None;

        template <typename Archive>
        static void Serialize(Archive& archive, TextureSettings& settings) {
            archive.Field("usage", settings.Usage);
            archive.Field("filter", settings.Filter);
            archive.Field("wrap", settings.Wrap);
            uint32_t isSrgb = settings.IsSRGB;
            archive.Field("IsSRGB", isSrgb);
            archive.Field("compression", settings.Compression);

            if constexpr (Archive::IsLoading) {
                settings.IsSRGB = isSrgb;
            }
        }
    };

    namespace TextureImporter {
        QS_API bool Load(LoadContext& loadContext);
        QS_API bool IsAssetSupported(std::string_view path);
        QS_API bool Import(ImportContext& importContext);

        inline EditorAssetImporterConfig GetImporterConfig() {
            return {
                .Type = Texture::GetStaticType(),
                .LoadAsset = Load,
                .IsAssetSupported = IsAssetSupported,
                .ImportAsset = Import,
            };
        }
    }
}
