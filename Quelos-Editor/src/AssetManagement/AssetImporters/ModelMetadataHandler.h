#pragma once

#include "Quelos/AssetManager/AssetMetadataPersistence.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct ModelMetadataHandler {
        static std::optional<AssetHandle> ReadAssetHandle(std::string_view assetPath);
        static bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);
        static bool SupportsAssetPath(std::string_view assetPath);
        static std::string GetMetadataFilePath(std::string_view assetPath);

    private:
        static std::optional<AssetHandle> ReadFromQuelFile(std::string_view quelPath) ;
        static bool WriteToQuelFile(std::string_view quelPath, const AssetHandle& handle) ;
    };
}
