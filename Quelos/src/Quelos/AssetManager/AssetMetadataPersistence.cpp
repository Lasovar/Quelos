#include "qspch.h"
#include "AssetMetadataPersistence.h"

namespace Quelos {
    AssetMetadataHandlerRegistry& AssetMetadataHandlerRegistry::Instance() {
        static AssetMetadataHandlerRegistry instance;
        return instance;
    }

    void AssetMetadataHandlerRegistry::RegisterHandler(const AssetMetadataHandlerFunctions& functions) {
        if (m_HandlerCount < MAX_HANDLERS) {
            m_Handlers[m_HandlerCount++] = functions;
        }
    }

    std::optional<AssetHandle> AssetMetadataHandlerRegistry::ReadAssetHandle(const std::string_view assetPath) const {
        const AssetMetadataHandlerFunctions* handler = FindHandler(assetPath);
        if (handler) {
            return handler->ReadAssetHandle(assetPath, handler->UserData);
        }
        return std::nullopt;
    }

    bool AssetMetadataHandlerRegistry::WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) const {
        const AssetMetadataHandlerFunctions* handler = FindHandler(assetPath);
        if (handler) {
            return handler->WriteAssetHandle(assetPath, handle, handler->UserData);
        }
        return false;
    }

    const AssetMetadataHandlerFunctions* AssetMetadataHandlerRegistry::FindHandler(const std::string_view assetPath) const {
        for (size_t i = 0; i < m_HandlerCount; ++i) {
            const auto& handler = m_Handlers[i];
            if (handler.SupportsAssetPath(assetPath, handler.UserData)) {
                return &handler;
            }
        }
        return nullptr;
    }
}
