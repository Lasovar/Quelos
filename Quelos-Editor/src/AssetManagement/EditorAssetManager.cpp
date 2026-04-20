#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/SceneImporter.h"
#include "EditorAssetImporter.h"
#include "AssetImporters/ShaderImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/Serializer.h"

namespace QuelosEditor {
    static const OsPath k_AssetRegistryFilename = "AssetRegistry.quel";

    const AssetMetadata* EditorAssetManager::AddAssetToRegistry(const std::string_view assetPath) {
        if (const AssetMetadata* metadata = m_AssetRegistry.GetAssetMetadata(assetPath)) {
            return metadata;
        }

        OsPath asset(assetPath);
        if (asset.is_absolute()) {
            asset = std::filesystem::relative(asset, Project::GetProjectPath());
        }

        const std::string relativePath = asset.generic_string();

        const AssetType assetType = AssetImporter::GetAssetType(relativePath);
        if (!assetType) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Failed to add asset '{}' to registry: unknown asset type!", assetPath
            );

            return nullptr;
        }

        AssetHandle handle;

        if (const AssetHandle existingHandle = EditorAssetImporter::ReadAssetHandle(relativePath)) {
            handle = existingHandle;
            QS_CORE_INFO_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Using existing asset handle {} for '{}'",
                handle.ToString(), assetPath
            );
        } else {
            handle = AssetHandle::Generate();
            QS_CORE_INFO_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Generated new asset handle {} for '{}'",
                handle.ToString(), assetPath
            );
            
            EditorAssetImporter::WriteAssetHandle(relativePath, handle);
        }

        const AssetMetadata assetMetadata = {
            handle,
            relativePath,
            assetType
        };

        return &(m_AssetRegistry.GetAssetsMetadata()[handle] = assetMetadata);
    }

    Vec<const AssetMetadata*> EditorAssetManager::ProcessAssetRegistration(const std::string_view assetPath) {
        Vec<const AssetMetadata*> registeredAssets;

        const AssetMetadata* mainAsset = AddAssetToRegistry(assetPath);
        if (!mainAsset) {
            return registeredAssets;
        }
        registeredAssets.push_back(mainAsset);

        const Vec<AssetMetadata> additionalAssets = AssetRegistryExtensions::ProcessAssetRegistration(
            assetPath, *mainAsset
        );

        for (const auto& additionalAsset : additionalAssets) {
            const AssetMetadata* addedMetadata = &(m_AssetRegistry.GetAssetsMetadata()[additionalAsset.Handle] = additionalAsset);
            registeredAssets.push_back(addedMetadata);
        }

        return registeredAssets;
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

    bool EditorAssetManager::IsAssetPathValid(const std::string_view path) const {
        return m_AssetRegistry.IsAssetPathValid(path);
    }

    Vec<const AssetMetadata*> EditorAssetManager::FindAssetsOfType(const AssetType& type) const {
        Vec<const AssetMetadata*> results;

        const HashMap<GUID128, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
        for (const AssetMetadata& metadata : assetsMetadata | std::views::values) {
            if (metadata.Type == type) {
                results.push_back(&metadata);
            }
        }

        return results;
    }

    bool EditorAssetManager::IsAssetSupported(const std::string_view path) {
        return AssetImporter::IsAssetSupported(path);
    }

    bool EditorAssetManager::IsAssetLoaded(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    EditorAssetManager::EditorAssetManager() {
        ModelImporter::Initialize();
        ShaderImporter::Initialize();
        AssetImporter::RegisterAssetImporter(SceneImporter::GetImporterConfig());
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
        
        if (metadata.IsSubAsset()) {
            if (Ref<Asset> subAsset = AssetRegistryExtensions::ResolveSubAsset(handle, metadata)) {
                m_LoadedAssets[handle] = subAsset;
                return subAsset;
            }
            
            QS_CORE_ERROR_TAG("AssetManager::GetAsset", 
                              "Failed to resolve sub-asset {} with parent {}", 
                              handle.ToString(), metadata.ParentHandle.ToString());
            return nullptr;
        }

        Ref<Asset> asset = AssetImporter::ImportAsset(handle, metadata);

        if (!asset) {
            QS_CORE_ERROR_TAG("AssetManager::GetAsset", "Failed to load asset with handle {}, path: '{}'",
                              handle.ToString(), metadata.FilePath);
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

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const std::string_view path) const {
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

        OsPath assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;

        std::string buffer;
        StringQuelWriter writer(buffer);
        for (auto& [handle, metadata] : m_AssetRegistry.GetAssetsMetadata()) {
            writer.Write(SectionEvent{metadata.Type.GetName()});
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString{handle.ToString()});
            writer.WriteField("path", metadata.FilePath);
            if (metadata.IsSubAsset()) {
                writer.WriteField("parent", UnquotedString{metadata.ParentHandle.ToFormattedString()});
                writer.WriteField("virtualPath", metadata.VirtualPath);
            }
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

        OsPath assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;
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

                    if (const AssetType assetTypeResult = GetAssetType(std::string(e.Name))) {
                        metadata.Type = assetTypeResult;
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
                            metadata.FilePath = *valueResult;
                        }
                        else if (currentField == "parent") {
                            metadata.ParentHandle = AssetHandle(*valueResult);
                        }
                        else if (currentField == "virtualPath") {
                            metadata.VirtualPath = *valueResult;
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
