//
// Created by lasovar on 10/13/26.
//

#include "ImportContext.hpp"

#include "xxhash.h"

#include "Quelos/Project/Project.h"
#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace QuelosEditor {
    static AssetID DeriveSubAssetID(const AssetID& parent, const std::string_view name) {
        static_assert(sizeof(AssetID) == 16 && std::is_trivially_copyable_v<AssetID>);

        const UniquePtr<XXH3_state_t, decltype(&XXH3_freeState)> state(XXH3_createState(), &XXH3_freeState);
        XXH3_128bits_reset(state.get());
        XXH3_128bits_update(state.get(), &parent, sizeof(parent));
        XXH3_128bits_update(state.get(), name.data(), name.size());

        XXH128_canonical_t canonical;
        XXH128_canonicalFromHash(&canonical, XXH3_128bits_digest(state.get()));

        std::array<uint8_t, 16> bytes;
        std::memcpy(bytes.data(), &canonical, 16);

        // Mark as a name-derived ID (UUID version 8, RFC 9562 variant)
        bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x80);
        bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

        AssetID id;
        std::memcpy(&id, bytes.data(), 16);
        return id;
    }

    void ImportContext::DependOn(const AssetID& id) {
        m_Dependencies.push_back(id);
    }

    void ImportContext::DependOn(const std::string_view projectRelativePath) {
        const AssetMetadata* metadata = m_Registry.GetAssetMetadata(projectRelativePath);
        if (metadata == nullptr) {
            QS_ERROR_TAG(
                "ImportContext::DependOn",
                "Failed to find asset metadata in path '{}'",
                projectRelativePath
            );

            return;
        }
        
        m_Dependencies.push_back(metadata->Handle);
    }

    Expected<void, String> ImportContext::WriteArtifact(std::string_view name, const BufferView data) {
        const OsPath& libraryPath = Project::GetLibraryPath();
        String assetId = m_Asset.Handle.ToString(Allocator::Frame);
        m_Artifacts.emplace_back(FormatTemp("Artifacts/{}{}/{}/{}", assetId[0], assetId[1], assetId, name));
        const OsPath targetPath = libraryPath / m_Artifacts.back().view();

        std::error_code ec;
        std::filesystem::create_directories(targetPath.parent_path(), ec);

        if (Utility::WriteFile(targetPath, data)) {
            return {};
        }

        m_Artifacts.pop_back();
        return Unexpected(String(FormatTemp("Failed to write file to '{}'!", targetPath.generic_string()), Allocator::Persistent));
    }

    AssetID ImportContext::AddSubAsset(const std::string_view stableName, const AssetType& type) {
        return AddSubAsset(stableName, DeriveSubAssetID(m_Asset.Handle, stableName), type);
    }

    AssetID ImportContext::AddSubAsset(std::string_view stableName, AssetID subAssetId, const AssetType& type) {
        m_SubAssets.emplace_back(
            subAssetId,
            FormatTemp("{}/{}", m_Asset.FilePath, stableName),
            type,
            m_Asset.Handle
        );

        return subAssetId;
    }
}
