#pragma once

#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetMetadata.h"
#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/Renderer/Texture.h"

namespace QuelosEditor {
    struct TextureSettings {
        enum class Usage {
            Default,
            Normal
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
            archive.Field("filter", settings.Filter);
            archive.Field("wrap", settings.Wrap);
            archive.Field("compression", settings.Compression);
        }
    };

    namespace TextureImporter {
        QS_API bool ImportTexture2D(LoadContext& loadContext);
        QS_API bool IsAssetSupported(std::string_view path);
        QS_API bool Import(ImportContext& importContext);

        inline EditorAssetImporterConfig GetImporterConfig() {
            return {
                .Type = Texture::GetStaticType(),
                .LoadAsset = ImportTexture2D,
                .IsAssetSupported = IsAssetSupported,
                .ImportAsset = Import,
            };
        }
    }
}
