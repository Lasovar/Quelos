#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporter.h"
#include "magic_enum/magic_enum.hpp"
#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/Serializer.h"

namespace Quelos {
    static const Path k_AssetRegistryFilename = "AssetRegistry.quel";

    AssetHandle EditorAssetManager::AddAssetToRegistry(const AssetType assetType, const Path& assetPath) {
        AssetHandle handle = AssetHandle::Generate();
        m_AssetRegistry.GetAssetsMetadata().emplace(handle, AssetMetadata{handle, assetPath, assetType});
        return handle;
    }

    bool EditorAssetManager::IsAssetHandleValid(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_AssetRegistry.IsAssetHandleValid(handle);
    }

    bool EditorAssetManager::IsAssetLoaded(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    Ref<Asset> EditorAssetManager::GetAsset(const AssetHandle& handle) {
        if (!IsAssetHandleValid(handle)) {
            QS_CORE_ERROR_TAG("AssetManager", "Invalid asset handle {}", handle.ToString());
            return nullptr;
        }

        if (IsAssetLoaded(handle)) {
            return m_LoadedAssets.at(handle);
        }

        const AssetMetadata& metadata = m_AssetRegistry.GetAssetMetadata(handle);
        Ref<Asset> asset = AssetImporter::ImportAsset(handle, metadata);

        if (!asset) {
            QS_CORE_ERROR_TAG("AssetManager::GetAsset", "Failed to load asset with handle {}", handle.ToString());
            return nullptr;
        }

        m_LoadedAssets[handle] = asset;

        return asset;
    }

    void EditorAssetManager::SerializeAssetRegistry() {
        using namespace Serialization;

        Path assetRegistryPath = Project::GetLibraryPath() / k_AssetRegistryFilename;

        std::string buffer;
        StringQuelWriter writer(buffer);
        for (auto& [handle, metadata] : m_AssetRegistry.GetAssetsMetadata()) {
            writer.Write(SectionEvent{magic_enum::enum_name(metadata.Type)});
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString{handle.ToString()});
            writer.WriteField("path", metadata.FilePath.string());
        }


        if (std::ofstream file(assetRegistryPath, std::ios::binary); file) {
            file.write(buffer.c_str(), buffer.size());
        }
        else {
            QS_CORE_ERROR_TAG(
                "AssetManager::SerializeAssetRegistry",
                "Failed to open registry file '{}'",
                assetRegistryPath.string()
            );
        }
    }

    void EditorAssetManager::DeserializeAssetRegistry() {
        using namespace Serialization;

        Path assetRegistryPath = Project::GetLibraryPath() / k_AssetRegistryFilename;
        std::ifstream file(assetRegistryPath, std::ios::binary | std::ios::ate);
        if (!file) {
            QS_CORE_ERROR_TAG(
                "AssetManager::DeserializeAssetRegistry",
                "Failed to open registry file '{}'",
                assetRegistryPath.string()
            );

            return;
        }

        const size_t fileSize = file.tellg();
        file.seekg(0);

        std::string assetRegistryBuffer;
        assetRegistryBuffer.resize(fileSize);
        file.read(assetRegistryBuffer.data(), fileSize);

        QuelReader reader(assetRegistryBuffer);

        AssetMetadata metadata;
        std::string_view currentField;

        HashMap<AssetHandle, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
        for (auto&& parserEvent : reader.Parse()) {
            std::visit([&]<typename TEvent>(const TEvent& e) {
                using T = std::decay_t<TEvent>;

                if constexpr (std::is_same_v<T, SectionEvent>) {
                    if (metadata.Handle) {
                        assetsMetadata[metadata.Handle] = metadata;
                        metadata = {};
                    }

                    if (auto assetTypeResult = magic_enum::enum_cast<AssetType>(e.Name)) {
                        metadata.Type = assetTypeResult.value();
                    }
                }
                else if constexpr (std::is_same_v<T, FieldEvent>) {
                    currentField = e.Path;
                }
                else if constexpr (std::is_same_v<T, ValueEvent>) {
                    if (const std::string_view* valueResult = std::get_if<std::string_view>(&e.Value)) {
                        if (currentField == "handle") {
                            metadata.Handle = AssetHandle(*valueResult);
                        }
                        else if (currentField == "path") {
                            metadata.FilePath = Path{*valueResult};
                        }
                    }
                }
            }, parserEvent);
        }

        if (metadata.Handle) {
            assetsMetadata[metadata.Handle] = metadata;
        }
    }
}
