#pragma once

#include "AssetImporter.h"
#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    struct TextureSettings {
        enum class CompressionQuality {
            None,
            Low,
            Normal,
            High
        };

        TextureType Type = TextureType::Texture2D;
        FilterMode Filter = FilterMode::Linear;
        WrapMode Wrap = WrapMode::Repeat;
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

        inline AssetImporterConfig GetImporterConfig() {
            return {Texture::GetStaticType(), ImportTexture2D, IsAssetSupported};
        }
    }
}
