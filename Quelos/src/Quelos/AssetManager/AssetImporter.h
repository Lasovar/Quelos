#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    using AssetLoaderFn = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;
    using IsAssetSupportedFn = std::function<bool(std::string_view)>;

    inline std::string_view Extension(const std::string_view path) {
        const size_t pos = path.find_last_of('.');
        if (pos == std::string_view::npos) {
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return path;
        }

        return { path.data() + pos, path.size() - pos };
    }

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
        
        // Metadata handling functions for embedded handles
        using ReadHandleFn = std::optional<AssetHandle>(*)(std::string_view assetPath);
        using WriteHandleFn = bool(*)(std::string_view assetPath, const AssetHandle& handle);
        
        ReadHandleFn ReadAssetHandle = nullptr;
        WriteHandleFn WriteAssetHandle = nullptr;
    };

    namespace AssetImporter {
        QS_API void RegisterAssetImporter(const AssetImporterConfig& config);

        QS_API bool IsAssetSupported(std::string_view path);
        QS_API const AssetType& GetAssetType(std::string_view path);
        QS_API Ref<Asset> ImportAsset(AssetHandle assetHandle, const AssetMetadata& metadata);
        
        // Metadata handling functions - delegates to appropriate importer
        QS_API std::optional<AssetHandle> ReadAssetHandle(std::string_view assetPath);
        QS_API bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);
    }
}
