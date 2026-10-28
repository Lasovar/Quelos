#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

#include "Quelos/Core/Buffer.h"
#include "Quelos/Serialization/QuelArchive.h"

namespace Quelos {
    class LoadContext {
    public:
        explicit LoadContext(
            const AssetMetadata& metadata,
            void* slot
        ) : m_Metadata(metadata), m_LoadSlot(slot) {}

        explicit LoadContext(
            const AssetMetadata& metadata,
            Serialization::QuelAutoReadArchive& settings,
            void* slot
        ) : m_Metadata(metadata),
            m_ReadArchive(std::in_place, Ref(settings)),
            m_LoadSlot(slot) {}

        [[nodiscard]] const AssetMetadata& GetAssetMetadata() const { return m_Metadata; }
        [[nodiscard]] void* GetLoadSlot() const { return m_LoadSlot; }
        [[nodiscard]] Optional<Buffer> ReadArtifact(std::string_view name) const;

        template <typename T>
        [[nodiscard]] Optional<T> Settings() {
            Optional<T> settings = None;

            if (!m_ReadArchive) {
                return settings;
            }

            settings.emplace();
            m_ReadArchive->get().Section("Settings");
            T::Serialize(m_ReadArchive->get(), *settings);

            return settings;
        }

    private:
        const AssetMetadata& m_Metadata;
        Optional<Ref<Serialization::QuelAutoReadArchive>> m_ReadArchive;
        void* m_LoadSlot = nullptr;
    };

    using AssetLoaderFn = std::function<bool(LoadContext&)>;
    using IsAssetSupportedFn = std::function<bool(std::string_view)>;

    struct QS_API AssetImporterConfig {
        AssetType Type = {};
        AssetLoaderFn LoadAsset;
        IsAssetSupportedFn IsAssetSupported;
    };

    namespace AssetImporter {
        QS_API void RegisterAssetImporter(const AssetImporterConfig& config);

        QS_API Optional<Ref<const AssetType>> GetAssetType(AssetTypeID assetTypeId);
        QS_API bool IsAssetSupported(std::string_view path);
        QS_API const AssetType& GetAssetType(std::string_view path);
        QS_API bool LoadAsset(LoadContext& loadContext);
        QS_API uint32_t GetVersion(AssetTypeID assetTypeId);
    }
}
