#pragma once

#include "AssetMetadata.h"
#include <optional>

namespace Quelos {
    // Function table for asset metadata handlers
    struct AssetMetadataHandlerFunctions {
        using ReadAssetHandleFn = std::optional<AssetHandle>(*)(const std::string_view assetPath, void* userData);
        using WriteAssetHandleFn = bool(*)(const std::string_view assetPath, const AssetHandle& handle, void* userData);
        using SupportsAssetPathFn = bool(*)(const std::string_view assetPath, void* userData);
        using GetMetadataFilePathFn = std::string(*)(const std::string_view assetPath, void* userData);
        
        ReadAssetHandleFn ReadAssetHandle;
        WriteAssetHandleFn WriteAssetHandle;
        SupportsAssetPathFn SupportsAssetPath;
        GetMetadataFilePathFn GetMetadataFilePath;
        void* UserData;
    };

    // Registry for metadata handlers - no heap allocations
    class AssetMetadataHandlerRegistry {
    public:
        static AssetMetadataHandlerRegistry& Instance();
        
        // Register handler with function table - no heap allocation
        void RegisterHandler(const AssetMetadataHandlerFunctions& functions);
        
        // Try to read existing asset handle from any registered handler
        std::optional<AssetHandle> ReadAssetHandle(const std::string_view assetPath) const;
        
        // Write asset handle using the appropriate handler
        bool WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) const;
        
    private:
        static constexpr size_t MAX_HANDLERS = 16;
        AssetMetadataHandlerFunctions m_Handlers[MAX_HANDLERS];
        size_t m_HandlerCount = 0;
        
        // Find the appropriate handler for the given asset path
        const AssetMetadataHandlerFunctions* FindHandler(const std::string_view assetPath) const;
    };

    // Template helper for creating metadata handler function tables
    template<typename T>
    struct AssetMetadataHandler {
        static std::optional<AssetHandle> ReadAssetHandle(const std::string_view assetPath, void* userData) {
            T* instance = static_cast<T*>(userData);
            return instance->ReadAssetHandle(assetPath);
        }
        
        static bool WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle, void* userData) {
            T* instance = static_cast<T*>(userData);
            return instance->WriteAssetHandle(assetPath, handle);
        }
        
        static bool SupportsAssetPath(const std::string_view assetPath, void* userData) {
            T* instance = static_cast<T*>(userData);
            return instance->SupportsAssetPath(assetPath);
        }
        
        static std::string GetMetadataFilePath(const std::string_view assetPath, void* userData) {
            T* instance = static_cast<T*>(userData);
            return instance->GetMetadataFilePath(assetPath);
        }
        
        static AssetMetadataHandlerFunctions Create(T& instance) {
            return {
                ReadAssetHandle,
                WriteAssetHandle,
                SupportsAssetPath,
                GetMetadataFilePath,
                &instance
            };
        }
    };
}
