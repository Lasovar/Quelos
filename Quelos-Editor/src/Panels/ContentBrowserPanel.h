#pragma once

#include "imgui.h"
#include "Quelos/AssetManager/Asset.h"
#include "AssetManagement/EditorAssetManager.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct AssetEntry {
        bool IsImportable = false;
        AssetMetadata Metadata;
        std::string Name;

        struct Compare {
            using is_transparent = void;

            constexpr bool operator()(const AssetEntry& a, const AssetEntry& b) const {
                return a.Name < b.Name;
            }
        };
    };

    struct DirectoryData {
        std::string DirectoryPath;
        OrderedSet<AssetEntry, AssetEntry::Compare> Assets;
        OrderedSet<std::string> SubDirectories;
    };

    class ContentBrowserPanel {
    public:
        ContentBrowserPanel() = default;
        void Init();

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void DrawDirectoryTile(const std::string& path);
        void DrawAssetTile(AssetEntry& asset);
        void DrawTopBar();
        void DrawAssetGrid();
        void DrawDirectoryTree();
        void DrawDirectoryNode(const std::string& path);
        void IterateDirectory(const std::filesystem::directory_entry& directory);

        void RebuildDirectoryTree();

    private:
        std::string m_RootPath;
        std::string m_RelativeRootPath = ".";
        std::string m_CurrentPath = ".";

        OrderedMap<std::string, DirectoryData> m_Directories;
        bool m_QueueDirectoryTreeRebuild = false;
        Ref<EditorAssetManager> m_AssetManager;
    };
}
