#include "qspch.h"
#include "AssetImporter.h"

#include "magic_enum/magic_enum.hpp"

#include "Quelos/Core/Profiling.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace Quelos {
    static HashMap<AssetTypeID, AssetImporterConfig> s_AssetLoaders{Allocator::Persistent};

    void AssetImporter::RegisterAssetImporter(const AssetImporterConfig& config) {
        s_AssetLoaders[config.Type] = config;
    }

    Optional<Ref<const AssetType>> AssetImporter::GetAssetType(const AssetTypeID assetTypeId) {
        const auto it = s_AssetLoaders.find(assetTypeId);
        if (it == s_AssetLoaders.end()) {
            return None;
        }

        return it->second.Type;
    }

    bool AssetImporter::LoadAsset(LoadContext& loadContext) {
        QS_PROFILE_SCOPED_N("AssetImporter::ImportAsset");

        const auto it = s_AssetLoaders.find(loadContext.GetAssetMetadata().Type);
        if (it == s_AssetLoaders.end()) {
            QS_ERROR_TAGGED(
                "Failed to import asset '{}': No suitable importer",
                loadContext.GetAssetMetadata().FilePath
            );

            return false;
        }

        return it->second.LoadAsset(loadContext);
    }

    uint32_t AssetImporter::GetVersion(AssetTypeID assetTypeId) {
        return 0;
    }

    bool AssetImporter::IsAssetSupported(const std::string_view path) {
        return std::ranges::any_of(s_AssetLoaders | std::views::values,
            [&path](const AssetImporterConfig& importerConfig) {
                return importerConfig.IsAssetSupported(path);
            }
        );
    }

    const AssetType& AssetImporter::GetAssetType(const std::string_view path) {
        for (auto & importerConfig : s_AssetLoaders | std::views::values) {
            if (importerConfig.IsAssetSupported(path)) {
                return importerConfig.Type;
            }
        }

        return AssetType::Invalid;
    }

    // TODO: Implementation is heavily Editor focused, should use the asset pack virtual load
    // (Maybe Utility::ReadFile itself handles doing that)
    Optional<Buffer> LoadContext::ReadArtifact(std::string_view name) const {
        const OsPath& libraryPath = Project::GetLibraryPath();
        String assetId = m_Metadata.ID.ToString(Allocator::Frame);
        const OsPath targetPath = libraryPath / FormatTemp("Artifacts/{}{}/{}/{}", assetId[0], assetId[1], assetId, name);

        Buffer data = Utility::ReadFile(targetPath, false);
        if (!data) {
            return None;
        }

        return data;
    }
}
