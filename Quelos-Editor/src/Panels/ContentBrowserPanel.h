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
    };

    struct DirectoryData {
        Path DirectoryPath;
        Vec<AssetEntry> Assets;
        HashSet<Path> SubDirectories;
    };

    class ContentBrowserPanel {
    public:
        ContentBrowserPanel() = default;
        void Init();

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        void DrawFolderTile(const Path& path);
        void DrawAssetTile(AssetEntry& asset);
        void DrawTopBar();
        void DrawAssetGrid();
        void DrawFolderTree();
        void DrawFolderNode(const Path& path);
        void IterateDirectory(const std::filesystem::directory_entry& directory);

        void RebuildDirectoryTree();

    private:
        Path m_RootPath;
        Path m_RelativeRootPath;
        Path m_CurrentPath;

        OrderedMap<Path, DirectoryData> m_Directories;
        bool m_QueueDirectoryTreeRebuild = false;
        Ref<EditorAssetManager> m_AssetManager;
    };
}
