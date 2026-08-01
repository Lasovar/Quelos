#pragma once

#include "imgui.h"
#include "Quelos/AssetManager/Asset.h"
#include "AssetManagement/EditorAssetManager.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct AssetEntry {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        bool IsImportable = false;
        AssetMetadata Metadata;
        String Name;

        AssetEntry() = default;
        AssetEntry(const AssetEntry&) = delete;
        AssetEntry& operator=(const AssetEntry&) = delete;
        AssetEntry(AssetEntry&&) = default;
        AssetEntry& operator=(AssetEntry&&) = default;

        explicit AssetEntry(const allocator_type allocator)
            : Name(allocator) {}

        explicit AssetEntry(const AllocatorType allocatorType)
            : Name(allocatorType) {}

        explicit AssetEntry(AssetEntry&& other, const allocator_type allocator)
            : IsImportable(other.IsImportable), Metadata(std::move(other.Metadata)), Name(std::move(other.Name), allocator)
        {
            other.IsImportable = false;
        }

        struct Compare {
            constexpr bool operator()(const AssetEntry& a, const AssetEntry& b) const {
                return a.Name < b.Name;
            }
        };
    };

    struct DirectoryData {
        using allocator_type = std::pmr::polymorphic_allocator<>;

        SortedSet<AssetEntry, AssetEntry::Compare> Assets;
        SortedSet<String> SubDirectories;

        DirectoryData() = default;

        explicit DirectoryData(const allocator_type allocator)
            : Assets(allocator), SubDirectories(allocator) {}

        explicit DirectoryData(const AllocatorType allocator)
            : DirectoryData(GetAllocator(allocator)) {}

        explicit DirectoryData(DirectoryData&& other, const allocator_type allocator)
            : Assets(std::move(other.Assets), allocator), SubDirectories(std::move(other.SubDirectories), allocator) {}

        explicit DirectoryData(DirectoryData&& other, const AllocatorType allocatorType)
            : DirectoryData(std::move(other), GetAllocator(allocatorType)) {}
    };

    class ContentBrowserPanel {
    public:
        ContentBrowserPanel() = default;
        void Init();

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void DrawDirectoryTile(std::string_view path);
        void StartAssetRename(std::string_view assetName, AssetID assetId);
        void RenameAsset(const AssetEntry& asset, std::string_view newName);
        void DrawAssetTile(AssetEntry& asset);
        void DrawTopBar();
        void DrawAssetGrid();
        void DrawDirectoryTree();
        void DrawDirectoryNode(std::string_view path);
        void IterateDirectory(const std::filesystem::directory_entry& directory);

        void RebuildDirectoryTree();

    private:
        String m_RootPath{Allocator::Persistent};
        String m_RelativeRootPath{".", Allocator::Persistent};
        String m_CurrentPath{".", Allocator::Persistent};

        fmt::memory_buffer m_RenameBuffer;
        AssetID m_RenamingAsset;

        SortedMap<String, DirectoryData> m_Directories{Allocator::Persistent};
        bool m_QueueDirectoryTreeRebuild = false;
        SharedPtr<EditorAssetManager> m_AssetManager;
    };
}
