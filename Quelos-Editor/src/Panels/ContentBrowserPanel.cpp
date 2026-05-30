#include "qspch.h"
#include "ContentBrowserPanel.h"

#include "EditorLayer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "magic_enum/magic_enum.hpp"

#include "Quelos/ImGui/icons_font_awesome.h"
#include "Quelos/Project/Project.h"
#include "Quelos/ImGui/ImGuiUI.h"

#include "EditorUI.h"
#include "AssetManagement/AssetImporters/MaterialImporter.h"
#include "Quelos/Utility/FileSystem.h"

namespace QuelosEditor {

    void ContentBrowserPanel::DrawDirectoryTile(const std::string& path) {
        std::string_view directoryName = FS::Filename(path);
        ImGui::PushID(path.c_str());
        ImGui::BeginGroup();

        if (ImGui::ButtonEx(ICON_FA_FOLDER, ImVec2(80, 80), ImGuiButtonFlags_PressedOnDoubleClick)) {
            m_CurrentPath = path;
        }

        ImGui::TextWrapped("%s", FormatTemp("{}", directoryName));

        ImGui::EndGroup();
        ImGui::PopID();
    }

    void ContentBrowserPanel::DrawAssetTile(AssetEntry& asset) {
        ImGui::PushID(asset.Name.c_str());

        ImGui::BeginGroup();

        if (asset.IsImportable) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        // TODO: replace with asset image
        ImGui::Button(ICON_FA_FILE_IMAGE_O, ImVec2(80, 80));

        if (asset.IsImportable) {
            ImGui::PopStyleVar();
        }

        if (ImGui::BeginPopupContextItem("AssetContext")) {
            if (ImGui::MenuItem(FormatTemp("{} {}", ICON_FA_TRASH, "Delete"))) {
                m_AssetManager->RemoveAssetFromRegistry(asset.Metadata.Handle);
                m_QueueDirectoryTreeRebuild = true;

                // TODO: delete from file system
            }

            if (!asset.IsImportable && ImGui::MenuItem("Remove from registry")) {
                if (asset.Metadata.Handle) {
                    m_AssetManager->RemoveAssetFromRegistry(asset.Metadata.Handle);
                    m_QueueDirectoryTreeRebuild = true;
                }
            }

            if (!asset.IsImportable && ImGui::MenuItem("Reimport")) {
                if (asset.Metadata.Handle) {
                    m_AssetManager->Reimport(asset.Metadata.Handle);
                    m_QueueDirectoryTreeRebuild = true;
                }
            }

            if (asset.IsImportable && ImGui::MenuItem("Import")) {
                const Vec<const AssetMetadata*> assetsMetadata = m_AssetManager->ProcessAssetRegistration(
                    asset.Metadata.FilePath
                );

                if (!assetsMetadata.empty() && assetsMetadata[0] && assetsMetadata[0]->Handle) {
                    asset.Metadata = *assetsMetadata[0];
                    asset.IsImportable = false;
                    m_QueueDirectoryTreeRebuild = true;
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            EditorLayer::Get().OpenAssetWorkspace(asset.Metadata);
        }

        // Drag & Drop
        if (!asset.IsImportable && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(
                asset.Metadata.Type.GetName().c_str(),
                &asset.Metadata.Handle, sizeof(AssetID)
            );

            ImGui::TextUnformatted(asset.Name.c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::TextWrapped("%s", asset.Name.c_str());

        ImGui::EndGroup();

        ImGui::PopID();
    }

    void ContentBrowserPanel::DrawTopBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));

        if (ImGui::Button("Root")) {
            m_CurrentPath = m_RelativeRootPath;
        }

        size_t start = 0;
        for (size_t i = 0; i <= m_CurrentPath.size(); i++) {
            if (i != m_CurrentPath.size() && m_CurrentPath[i] != '/') {
                continue;
            }

            std::string_view part(m_CurrentPath.data() + start, i - start);

            if (part != ".") {
                ImGui::SameLine();
                ImGui::Text("%s", ICON_FA_ARROW_RIGHT);
            } else {
                continue;
            }

            ImGui::SameLine();

            if (ImGui::Button(FormatTemp("{}", part))) {
                const std::string_view path(m_CurrentPath.data(), i);
                m_CurrentPath = path;
                break;
            }

            start = i + 1;
        }

        ImGui::PopStyleVar();
    }

    void ContentBrowserPanel::DrawAssetGrid() {
        auto& directory = m_Directories[m_CurrentPath];

        constexpr float padding = 16.0f;
        constexpr float thumbnail_size = 80.0f;
        constexpr float cell_size = thumbnail_size + padding;

        const float panel_width = ImGui::GetContentRegionAvail().x;
        int column_count = static_cast<int>(panel_width / cell_size);
        if (column_count < 1) column_count = 1;

        ImGui::Columns(column_count, nullptr, false);

        for (auto& subDirectory : directory.SubDirectories) {
            DrawDirectoryTile(subDirectory);
            ImGui::NextColumn();
        }

        for (auto& assetEntry : directory.Assets) {
            DrawAssetTile(assetEntry);
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void ContentBrowserPanel::DrawDirectoryTree() {
        auto& root = m_Directories[m_RelativeRootPath];

        for (auto& directory : root.SubDirectories) {
            DrawDirectoryNode(directory);
        }
    }

    void ContentBrowserPanel::DrawDirectoryNode(const std::string& path) {
        ImGuiTreeNodeFlags flags = 0
            | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_DrawLinesToNodes
            | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (m_CurrentPath == path) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        auto& directory = m_Directories[path];
        const bool hasChildren = !directory.SubDirectories.empty();
        if (!hasChildren) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool open = ImGui::TreeNodeEx(
            FormatTemp("{} {}", ICON_FA_FOLDER, FS::Filename(path)),
            flags
        );

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_CurrentPath = path;
        }

        if (hasChildren && open) {
            for (auto& child : directory.SubDirectories) {
                DrawDirectoryNode(child);
            }

            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        if (UI::Begin("Content Browser", dockspaceID, windowClass)) {
            static float sidebar_width = 220.0f;

            if (ImGui::BeginChild(
                "##sidebar",
                ImVec2(sidebar_width, 0),
                ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders)
            ) {
                DrawDirectoryTree();
            } ImGui::EndChild();

            ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0);
            ImGui::SameLine();
            ImGui::PopStyleVar(1);

            if (ImGui::BeginChild("##main", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
                if (ImGui::BeginPopupContextWindow("MainViewContext")) {
                    if (ImGui::MenuItem(FormatTemp("{} {}", ICON_FA_FILE, "Create Scene"))) {
                        Ref<Scene> scene = CreateRef<Scene>("NewScene");
                        SceneSerializer serializer(scene, m_CurrentPath + "/NewScene");
                        serializer.Deserialize();
                        m_QueueDirectoryTreeRebuild = true;
                    }

                    if (ImGui::MenuItem(FormatTemp("{} {}", ICON_FA_PAINT_BRUSH, "Create Material"))) {
                        MaterialImporter::CreateDefaultMaterialAsset(m_CurrentPath, "NewMaterial");
                        //m_AssetManager->ProcessAssetRegistration();
                    }

                    ImGui::EndPopup();
                }

                DrawTopBar();
                DrawAssetGrid();
            } ImGui::EndChild();

        } UI::End();

        if (m_QueueDirectoryTreeRebuild) {
            RebuildDirectoryTree();
            m_QueueDirectoryTreeRebuild = false;
        }
    }

    void ContentBrowserPanel::IterateDirectory(const std::filesystem::directory_entry& directory) {
        if (!directory.is_directory()) {
            return;
        }

        const std::string path = std::filesystem::relative(directory.path(), m_RootPath).generic_string();

        for (auto& entry : std::filesystem::directory_iterator(directory)) {
            const std::string& relativePath = std::filesystem::relative(entry.path(), m_RootPath).generic_string();

            if (const auto* metadata = m_AssetManager->GetAssetMetadata(relativePath)) {
                AssetEntry assetEntry;
                assetEntry.IsImportable = false;
                assetEntry.Metadata = *metadata;
                assetEntry.Name = FS::Filename(metadata->FilePath);

                m_Directories[path].Assets.insert(assetEntry);
            }
            else if (EditorAssetManager::IsAssetSupported(relativePath)) {
                AssetEntry assetEntry;
                assetEntry.IsImportable = true;
                assetEntry.Name = FS::Filename(relativePath);
                assetEntry.Metadata.FilePath = relativePath;

                m_Directories[path].Assets.insert(assetEntry);
            }
            else if (entry.is_directory() && entry.path() != Project::GetLibraryPath() && entry.path() !=
                Project::GetProjectSettingsPath()) {
                m_Directories[path].SubDirectories.emplace(relativePath);

                IterateDirectory(entry);
            }
        }
    }

    void ContentBrowserPanel::Init() {
        m_AssetManager = RefAs<EditorAssetManager>(Project::GetAssetManager());
        const auto& rootPath = Project::GetProjectPath();
        m_RootPath = rootPath.generic_string();
        m_RelativeRootPath = std::filesystem::relative(rootPath, rootPath).generic_string();

        RebuildDirectoryTree();

        m_CurrentPath = m_RelativeRootPath;
    }

    void ContentBrowserPanel::RebuildDirectoryTree() {
        m_Directories.clear();

        IterateDirectory(std::filesystem::directory_entry(m_RootPath));
    }
}
