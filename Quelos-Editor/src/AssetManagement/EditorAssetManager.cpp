#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/SceneImporter.h"
#include "EditorAssetImporter.h"
#include "AssetImporters/MaterialImporter.h"
#include "AssetImporters/ShaderImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/AssetManager/TextureImporter.h"
#include "Quelos/ImGui/ImGuiUI.h"
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

        const std::string relativePath = asset.lexically_normal().generic_string();

        const AssetType assetType = AssetImporter::GetAssetType(relativePath);
        if (!assetType) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Failed to add asset '{}' to registry: unknown asset type!", assetPath
            );

            return nullptr;
        }

        AssetID handle;

        if (const AssetID existingHandle = EditorAssetImporter::ReadAssetHandle(relativePath)) {
            handle = existingHandle;
            QS_CORE_INFO_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Using existing asset handle {} for '{}'",
                handle.ToString(), assetPath
            );
        }
        else {
            handle = AssetID::Generate();
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
            const AssetMetadata* addedMetadata = &(m_AssetRegistry.GetAssetsMetadata()[additionalAsset.Handle] =
                additionalAsset);
            registeredAssets.push_back(addedMetadata);
        }

        return registeredAssets;
    }

    void EditorAssetManager::RemoveAssetFromRegistry(const AssetID& assetHandle) {
        m_AssetRegistry.RemoveAsset(assetHandle);
        m_WatchedAssets.erase(assetHandle);
    }

    void EditorAssetManager::Reimport(const AssetID assetId) {
        m_ReimportQueue.push_back(assetId);
    }

    void EditorAssetManager::FlushReimportQueue() {
        for (const AssetID& assetId : m_ReimportQueue) {
            const AssetMetadata* metadata = GetAssetMetadata(assetId);
            if (!metadata) [[unlikely]] {
                QS_CORE_ERROR_TAG(
                    "EditorAssetManager",
                    "Failed to reimport asset! couldn't find asset metadata with AssetID {}",
                    assetId.ToString()
                );

                continue;
            }

            const auto assetLoaded = m_LoadedAssets.find(assetId);
            if (assetLoaded != m_LoadedAssets.end()) {
                const auto assetHandle = assetLoaded->second;
                const auto poolIt = m_AssetPools.find(assetHandle.Type);
                if (poolIt == m_AssetPools.end()) {
                    continue;
                }

                if (const auto pool = poolIt->second; pool.IsValid(pool.Data, assetHandle)) {
                    pool.ReleaseAt(pool.Data, assetHandle);
                    Import(pool, assetHandle, *metadata);
                }
            }
        }

        m_ReimportQueue.clear();
    }

    bool EditorAssetManager::Import(const UntypedAssetPool& pool, UntypedAssetHandle assetHandle, const AssetMetadata& metadata) {
        const AssetID assetId = metadata.Handle;
        void* slotData = pool.GetSlotData(pool.Data, assetHandle);

        if (metadata.IsSubAsset()) {
            if (AssetRegistryExtensions::ResolveSubAsset(slotData, metadata)) {
                pool.SetConstructed(pool.Data, assetHandle, true);
                m_LoadedAssets[assetId] = assetHandle;
                return true;
            }

            QS_CORE_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "Failed to resolve sub-asset {} with parent {}",
                assetId.ToString(), metadata.ParentId.ToString()
            );

            pool.DestroyAt(pool.Data, assetHandle);
            return false;
        }

        if (!EditorAssetImporter::ImportAsset(slotData, metadata)) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "Failed to load asset with handle {}, path: '{}'",
                assetId.ToString(), metadata.FilePath
            );

            pool.DestroyAt(pool.Data, assetHandle);
            return false;
        }

        pool.SetConstructed(pool.Data, assetHandle, true);
        m_LoadedAssets[assetId] = assetHandle;
        return true;
    }

    UntypedAssetHandle EditorAssetManager::Acquire(const AssetID assetId) {
        if (!IsAssetHandleValid(assetId)) {
            QS_CORE_ERROR_TAG("AssetManager", "Invalid asset handle {}", assetId.ToString());
            return {};
        }

        if (const auto it = m_LoadedAssets.find(assetId); it != m_LoadedAssets.end()) {
            if (IsAlive(it->second)) {
                return it->second;
            }

            m_LoadedAssets.erase(it);
        }

        const AssetMetadata& metadata = *m_AssetRegistry.GetAssetMetadata(assetId);

        const auto it = m_AssetPools.find(metadata.Type);
        if (it == m_AssetPools.end()) {
            QS_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "No suitable asset pool found for asset type '{}'!",
                metadata.Type.GetName()
            );

            return {};
        }

        const auto pool = it->second;
        const UntypedAssetHandle assetHandle = pool.Allocate(pool.Data);
        if (!Import(pool, assetHandle, metadata)) {
            return {};
        }

        return assetHandle;
    }

    void EditorAssetManager::Release(const UntypedAssetHandle assetHandle) {
        const auto it = m_AssetPools.find(assetHandle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto pool = it->second;
        pool.DestroyAt(pool.Data, assetHandle);
    }

    void EditorAssetManager::Release(const AssetID assetId) {
        const auto it = m_LoadedAssets.find(assetId);
        if (it == m_LoadedAssets.end()) {
            return;
        }

        Release(it->second);
        m_LoadedAssets.erase(it);
    }

    bool EditorAssetManager::IsAlive(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return false;
        }

        const auto& pool = it->second;
        return pool.IsValid(pool.Data, handle);
    }

    void EditorAssetManager::IncRef(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto& pool = it->second;
        pool.IncRef(pool.Data, handle);
    }

    void EditorAssetManager::DecRef(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto& pool = it->second;
        pool.DecRef(pool.Data, handle);
    }

    void EditorAssetManager::CleanupAssetMap() {
        // TODO: Is this even safe??
        for (auto it = m_LoadedAssets.begin(); it != m_LoadedAssets.end();) {
            if (!IsAlive(it->second)) {
                it = m_LoadedAssets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    bool EditorAssetManager::IsAssetHandleValid(const AssetID& handle) const {
        if (!handle) {
            return false;
        }

        return m_AssetRegistry.IsAssetHandleValid(handle);
    }

    bool EditorAssetManager::IsAssetPathValid(const std::string_view path) const {
        return m_AssetRegistry.IsAssetPathValid(path);
    }

    Vec<const AssetMetadata*> EditorAssetManager::FindAssetsOfType(const AssetTypeID type) const {
        Vec<const AssetMetadata*> results;

        const HashMap<AssetID, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
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

    bool EditorAssetManager::IsAssetLoaded(const AssetID& handle) const {
        if (!handle) {
            return false;
        }

        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    EditorAssetManager::EditorAssetManager() {
        ModelImporter::Initialize();
        RegisterType<Model>();
        RegisterType<Mesh>();

        ShaderImporter::Initialize();
        RegisterType<GraphicsShader>();

        MaterialImporter::Initialize();
        RegisterType<Material>();

        AssetImporter::RegisterAssetImporter(SceneImporter::GetImporterConfig());

        AssetImporter::RegisterAssetImporter(TextureImporter::GetImporterConfig());
        RegisterType<Texture2D>();
    }

    void EditorAssetManager::UnloadAsset(const AssetID assetHandle) {
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

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const AssetID& assetHandle) const {
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
            if (metadata.IsSubAsset()) {
                writer.WriteField("parent", UnquotedString{metadata.ParentId.ToFormattedString()});
            }
            writer.WriteField("path", metadata.FilePath);
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

        HashMap<AssetID, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
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
                            metadata.Handle = AssetID(*valueResult);
                        }
                        else if (currentField == "path") {
                            metadata.FilePath = *valueResult;
                        }
                        else if (currentField == "parent") {
                            metadata.ParentId = AssetID(*valueResult);
                        }
                    }
                }
            }, parserEvent);
        }

        if (metadata.Handle) {
            assetsMetadata[metadata.Handle] = metadata;
        }

        m_WatchID = m_FileWatcher.addWatch(
            (Project::GetAssetsPath()).generic_string(),
            this,
            true
        );

        m_FileWatcher.watch();
    }

    void EditorAssetManager::handleFileAction(const efsw::WatchID watchId, const std::string& dir,
                                              const std::string& filename, const efsw::Action action,
                                              std::string oldFilename) {
        switch (action) {
        case efsw::Action::Add:
        case efsw::Action::Delete:
            break;
        case efsw::Action::Modified: {
            const AssetMetadata* metadata = GetAssetMetadata(
                std::filesystem::relative(
                    FormatTemp("{}/{}", dir, filename),
                    Project::GetProjectPath()
                ).generic_string()
            );

            if (!metadata || metadata->ParentId) {
                break;
            }

            m_ReimportQueue.push_back(metadata->Handle);
            break;
        }
        default:
            break;
        }
    }
}
