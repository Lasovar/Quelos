#pragma once

#include "Quelos/AssetManager/AssetRef.h"
#include "Quelos/ImGui/ImGuiUI.h"
#include "Quelos/Utility/FileSystem.h"
#include "rapidfuzz/fuzz.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    namespace UI {
        using namespace Quelos::UI;

        inline bool Begin(std::string_view name, const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass,
                          bool* enabled = nullptr, ImGuiWindowFlags flags = 0) {
            ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowClass(&windowClass);

            flags |= ImGuiWindowFlags_NoNavFocus;

            return ImGui::Begin(FormatTemp("{}##{}", name, windowClass.ClassId), enabled, flags);
        }

        inline void End() {
            ImGui::End();
        }

        template <typename T>
            requires (std::is_base_of_v<Asset, T>)
        bool EditAsset(const std::string& label, AssetID& value) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            // Layout
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);

            // Frame style
            ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 4);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label.c_str());
            ImGui::NextColumn();

            ImGui::BeginGroup();

            // Asset display name
            static std::string typeName(T::GetStaticType().GetName());
            static std::string none = fmt::format("None ({})", typeName);

            static std::string assetName;

            // TODO: Maybe find a way to cache this instead of calling it every frame?
            if (value) {
                if (const AssetMetadata* meta = Project::GetAssetManager()->GetAssetMetadata(value)) {
                    assetName = FS::Filename(meta->FilePath);
                }
            }

            constexpr float resetButtonSize = 30.0f;
            const ImVec2 size = {
                ImGui::GetContentRegionAvail().x - (resetButtonSize * 2.0f) - 4.0f * 3.0f,
                ImGui::GetFrameHeight()
            };

            ImGui::Dummy(size);

            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();

            ImGui::RenderFrame(
                min,
                max,
                ImGui::GetColorU32(ImGuiCol_FrameBg),
                true,
                ImGui::GetStyle().FrameRounding
            );

            ImGui::RenderTextClipped(
                ImVec2(min.x + ImGui::GetStyle().FramePadding.x, min.y),
                max,
                value ? FormatTemp("{} ({})", assetName, typeName) : none.c_str(),
                nullptr,
                nullptr,
                ImVec2(0.0f, 0.5f)
            );

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(typeName.c_str())) {
                    if (const AssetID handle = *static_cast<const AssetID*>(payload->Data)) {
                        value = handle;
                        changed = true;
                    }
                }

                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();

            // Reset button
            if (ImGui::Button(ICON_FA_TRASH, ImVec2(resetButtonSize, 0.0f))) {
                value = {};
                changed = true;
            }

            ImGui::SameLine();

            static Vec<const AssetMetadata*> searchAssetMetadata;
            // Search button
            if (ImGui::Button("...", ImVec2(resetButtonSize, 0.0f))) {
                searchAssetMetadata = AssetManager::FindAssetsOfType<T>();
                ImGui::OpenPopup("AssetSearchPopup");
            }

            // Search Popup
            if (ImGui::BeginPopup("AssetSearchPopup")) {
                struct AssetSearchResult {
                    const AssetMetadata* Metadata = nullptr;
                    std::string_view Name{};
                    double Score = 0.0f;
                };

                static Vec<AssetSearchResult> results;
                results.clear();

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }

                static fmt::memory_buffer buffer;
                buffer.clear();
                ImGui::InputText(
                    FormatTemp("{}##assetSearch", ICON_FA_SEARCH),
                    buffer.data(),
                    buffer.capacity()
                );

                const std::string_view query = buffer.data();
                for (const AssetMetadata* metadata : searchAssetMetadata) {
                    std::string_view name = FS::Filename(metadata->FilePath);

                    const double nameScore = rapidfuzz::fuzz::WRatio(query, name);
                    const double pathScore = std::max({
                        rapidfuzz::fuzz::partial_ratio(query, metadata->FilePath),
                        rapidfuzz::fuzz::token_set_ratio(query, metadata->FilePath)
                    });

                    if (double score = 0.8f * nameScore + 0.2f * pathScore; score > 10.0f) {
                        results.push_back({metadata, name, score});
                    }
                }

                if (!results.empty()) {
                    std::ranges::sort(
                        results,
                        [](const AssetSearchResult& a, const AssetSearchResult& b) { return a.Score > b.Score; }
                    );
                }
                else {
                    for (const AssetMetadata* metadata : searchAssetMetadata) {
                        std::string_view name = FS::Filename(metadata->FilePath);

                        results.push_back({metadata, name, 0});
                    }
                }

                static uint32_t selectedIndex = 0;

                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                    selectedIndex = std::min(
                        selectedIndex + 1,
                        static_cast<uint32_t>(results.size()) - 1
                    );
                }

                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                    selectedIndex = std::max(selectedIndex - 1, 0u);
                }

                // Enter adds first result
                if (ImGui::IsKeyPressed(ImGuiKey_Enter) &&
                    !results.empty() &&
                    !ImGui::IsAnyItemHovered())
                {
                    value = results[selectedIndex].Metadata->Handle;
                    changed = true;

                    ImGui::CloseCurrentPopup();
                }

                for (uint32_t i = 0; i < results.size(); i++) {
                    const bool selected = selectedIndex == i;

                    if (ImGui::Selectable(FormatTemp("{}", FS::Filename(results[i].Name)), selected)) {
                        value = results[i].Metadata->Handle;
                        changed = true;

                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::EndGroup();

            ImGui::PopStyleVar(1);

            ImGui::Columns(1);
            ImGui::PopID();

            return changed;
        }
    }
}
