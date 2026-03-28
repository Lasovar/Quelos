#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporter.h"
#include "magic_enum/magic_enum.hpp"
#include "Quelos/Core/Formatters.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/Serializer.h"

namespace Quelos {
    static const Path k_AssetRegistryFilename = "AssetRegistry.quel";

    const AssetMetadata* EditorAssetManager::AddAssetToRegistry(const Path& assetPath) {
        const AssetHandle handle = AssetHandle::Generate();
        const Path& projectPath = Project::GetProjectPath();

        const AssetType assetType = AssetImporter::GetAssetType(assetPath);
        if (assetType == AssetType::None) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Failed to add asset '{}' to registry: unknow asset type!", assetPath.c_str()
            );

            return nullptr;
        }

        const AssetMetadata assetMetadata = {
            handle,
            std::filesystem::relative(assetPath, projectPath),
            assetType
        };

        return &(m_AssetRegistry.GetAssetsMetadata()[handle] = assetMetadata);
    }

    void EditorAssetManager::RemoveAssetFromRegistry(const AssetHandle& assetHandle) {
        m_AssetRegistry.RemoveAsset(assetHandle);
    }

    void EditorAssetManager::CleanupAssetMap() {
        for (auto it = m_LoadedAssets.begin(); it != m_LoadedAssets.end();) {
            if (it->second.expired()) {
                it = m_LoadedAssets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    bool EditorAssetManager::IsAssetHandleValid(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_AssetRegistry.IsAssetHandleValid(handle);
    }

    bool EditorAssetManager::IsAssetPathValid(const Path& path) const {
        return m_AssetRegistry.IsAssetPathValid(path);
    }

    bool EditorAssetManager::IsAssetSupported(const Path& path) {
        return AssetImporter::IsAssetSupported(path);
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
            if (auto existing = m_LoadedAssets.at(handle).lock()) {
                return existing;
            }
        }

        const AssetMetadata& metadata = *m_AssetRegistry.GetAssetMetadata(handle);
        Ref<Asset> asset = AssetImporter::ImportAsset(handle, metadata);

        if (!asset) {
            QS_CORE_ERROR_TAG("AssetManager::GetAsset", "Failed to load asset with handle {}, path: '{}'",
                              handle.ToString(), metadata.FilePath.string());
            return nullptr;
        }

        m_LoadedAssets[handle] = asset;

        return asset;
    }

    void EditorAssetManager::UnloadAsset(const AssetHandle assetHandle) {
        if (!IsAssetHandleValid(assetHandle)) {
            return;
        }

        m_LoadedAssets.erase(assetHandle);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const Path& path) const {
        if (!IsAssetPathValid(path)) {
            return nullptr;
        }

        return m_AssetRegistry.GetAssetMetadata(path);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const AssetHandle& assetHandle) const {
        if (!IsAssetHandleValid(assetHandle)) {
            return nullptr;
        }

        return m_AssetRegistry.GetAssetMetadata(assetHandle);

    }

    void EditorAssetManager::SerializeAssetRegistry() {
        using namespace Serialization;

        Path assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;

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

        Path assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;
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
