#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    using AssetLoaderFn = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;
    using IsAssetSupportedFn = std::function<bool(std::string_view)>;

    inline std::string NormalizeExt(const std::string_view ext) {
        std::string result(ext);

        if (!result.empty() && result[0] != '.') {
            result.insert(result.begin(), '.');
        }

        std::ranges::transform(result, result.begin(), tolower);
        return result;
    }

    struct QS_API AssetImporterConfig {
        AssetType Type = {};
        AssetLoaderFn LoadAsset;
        IsAssetSupportedFn IsAssetSupported;
    };

    namespace AssetImporter {
        QS_API void RegisterAssetImporter(const AssetImporterConfig& config);

        QS_API bool IsAssetSupported(std::string_view path);
        QS_API const AssetType& GetAssetType(std::string_view path);
        QS_API Ref<Asset> ImportAsset(AssetHandle assetHandle, const AssetMetadata& metadata);
    }
}
