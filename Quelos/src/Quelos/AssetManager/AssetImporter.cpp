#include "qspch.h"
#include "AssetImporter.h"

#include "TextureImporter.h"
#include "magic_enum/magic_enum.hpp"

namespace Quelos {
    using AssetLoaderFn = std::function<Ref<Asset>(const AssetMetadata&)>;
    static HashMap<AssetType, AssetLoaderFn> s_AssetLoaders = {
        {AssetType::Texture2D, TextureImporter::ImportTexture2D}
    };

    Ref<Asset> AssetImporter::ImportAsset(const AssetHandle assetHandle, const AssetMetadata& metadata) {
        const auto it = s_AssetLoaders.find(metadata.Type);
        if (it == s_AssetLoaders.end()) {
            QS_CORE_ERROR_TAG(
                "AssetImport::ImportAsset", "Failed to import asset '{}': No suitable importer",
                metadata.FilePath.string()
            );

            return nullptr;
        }

        return it->second(metadata);
    }
}
