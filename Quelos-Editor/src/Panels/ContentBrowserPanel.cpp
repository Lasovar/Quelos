#include "qspch.h"
#include "ContentBrowserPanel.h"

#include "EditorLayer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "magic_enum/magic_enum.hpp"

#include "Quelos/ImGui/icons_font_awesome.h"
#include "Quelos/Project/Project.h"
#include "Quelos/ImGui/ImGuiUI.h"

namespace QuelosEditor {
    void ContentBrowserPanel::DrawFolderTile(const Path& path) {
        ImGui::PushID(path.c_str());
        ImGui::BeginGroup();

        if (ImGui::ButtonEx(ICON_FA_FOLDER, ImVec2(80, 80), ImGuiButtonFlags_PressedOnDoubleClick)) {
            m_CurrentPath = path;
        }

        ImGui::TextWrapped("%s", path.filename().c_str());

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
            if (ImGui::MenuItem(UI::FormatTemp("{} {}", ICON_FA_TRASH, "Delete"))) {
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

            if (asset.IsImportable && ImGui::MenuItem("Import")) {
                const AssetMetadata* metadata = m_AssetManager->AddAssetToRegistry(
                    asset.Metadata.FilePath
                );

                if (metadata && metadata->Handle) {
                    asset.Metadata = *metadata;
                    asset.IsImportable = false;
                    m_QueueDirectoryTreeRebuild = true;
                }
            }

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            //TODO: OpenAsset(asset.Metadata.Handle);
            if (asset.Metadata.Type == AssetType::Scene) {
                EditorLayer::Get().OpenSceneWorkspace(asset.Metadata.Handle);
            }
        }

        // Drag & Drop
        if (!asset.IsImportable && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(
                std::string(magic_enum::enum_name(asset.Metadata.Type)).c_str(),
                &asset.Metadata.Handle, sizeof(AssetHandle)
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

        static std::string accumulated;
        accumulated.clear();

        if (ImGui::Button("Root")) {
            m_CurrentPath = m_RelativeRootPath;
        }

        for (auto& part : m_CurrentPath) {
            if (strcmp(part.c_str(), ".") != 0) {
                ImGui::SameLine();
                ImGui::Text("%s", ICON_FA_ARROW_RIGHT);
            } else {
                continue;
            }

            ImGui::SameLine();

            if (!accumulated.empty()) {
                accumulated.push_back('/');
            }

            accumulated += part.c_str();

            if (ImGui::Button(part.c_str())) {
                m_CurrentPath = accumulated;
                break;
            }
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

        for (auto& subfolder : directory.SubDirectories) {
            DrawFolderTile(subfolder);
            ImGui::NextColumn();
        }

        for (auto& assetEntry : directory.Assets) {
            DrawAssetTile(assetEntry);
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void ContentBrowserPanel::DrawFolderTree() {
        auto& root = m_Directories[m_RelativeRootPath];

        for (auto& folder : root.SubDirectories) {
            DrawFolderNode(folder);
        }
    }

    void ContentBrowserPanel::DrawFolderNode(const Path& path) {
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
            UI::FormatTemp("{} {}", ICON_FA_FOLDER, path.filename().c_str()),
            flags
        );

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_CurrentPath = path;
        }

        if (hasChildren && open) {
            for (auto& child : directory.SubDirectories) {
                DrawFolderNode(child);
            }

            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) {
        ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowClass(&windowClass);

        if (ImGui::Begin(UI::FormatTemp("Content Browser##{}", static_cast<void*>(this)))) {
            static float sidebar_width = 220.0f;

            if (ImGui::BeginChild(
                "##sidebar",
                ImVec2(sidebar_width, 0),
                ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders)
            ) {
                DrawFolderTree();
            } ImGui::EndChild();

            ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0);
            ImGui::SameLine();
            ImGui::PopStyleVar(1);

            if (ImGui::BeginChild("##main", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
                DrawTopBar();
                DrawAssetGrid();
            } ImGui::EndChild();

        } ImGui::End();

        if (m_QueueDirectoryTreeRebuild) {
            RebuildDirectoryTree();
            m_QueueDirectoryTreeRebuild = false;
        }
    }

    void ContentBrowserPanel::IterateDirectory(const std::filesystem::directory_entry& directory) {
        if (!directory.is_directory()) {
            return;
        }

        const Path path = std::filesystem::relative(directory.path(), m_RootPath);
        m_Directories[path].DirectoryPath = directory.path();

        for (auto& entry : std::filesystem::directory_iterator(directory)) {
            const auto& relativePath = std::filesystem::relative(entry.path(), m_RootPath);

            if (const auto* metadata = m_AssetManager->GetAssetMetadata(relativePath)) {
                AssetEntry assetEntry;
                assetEntry.IsImportable = false;
                assetEntry.Metadata = *metadata;
                assetEntry.Name = metadata->FilePath.filename();

                m_Directories[path].Assets.push_back(assetEntry);
            }
            else if (EditorAssetManager::IsAssetSupported(entry.path())) {
                AssetEntry assetEntry;
                assetEntry.IsImportable = true;
                assetEntry.Name = entry.path().filename();
                assetEntry.Metadata.FilePath = entry.path();

                m_Directories[path].Assets.push_back(assetEntry);
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
        m_RootPath = Project::GetProjectPath();
        m_RelativeRootPath = std::filesystem::relative(m_RootPath, m_RootPath);

        RebuildDirectoryTree();

        m_CurrentPath = m_RelativeRootPath;
    }

    void ContentBrowserPanel::RebuildDirectoryTree() {
        m_Directories.clear();

        IterateDirectory(std::filesystem::directory_entry(m_RootPath));
    }
}
