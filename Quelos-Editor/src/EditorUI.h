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
                          bool* enabled = nullptr, const ImGuiWindowFlags flags = 0) {
            ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowClass(&windowClass);

            return ImGui::Begin(FormatTemp("{}##{}", name, windowClass.ClassId), enabled, flags);
        }

        inline void End() {
            ImGui::End();
        }

        template <typename T>
            requires (std::is_base_of_v<Asset, T>)
        bool EditAsset(const std::string& label, AssetRef<T>& value) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            // Layout
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3.0f);

            // Frame style
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label.c_str());
            ImGui::NextColumn();

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::BeginGroup();

            // Asset display name
            static std::string typeName(T::GetStaticType().GetName());
            static std::string none = fmt::format("None ({})", typeName);

            static std::string assetName;

            // TODO: Maybe find a way to cache this instead of calling it every frame?
            if (value) {
                if (const AssetMetadata* meta = Project::GetAssetManager()->
                    GetAssetMetadata(value.GetAssetID())) {
                    assetName = FS::Filename(meta->FilePath);
                }
            }

            constexpr float resetButtonSize = 30.0f;
            const ImVec2 size = {ImGui::GetContentRegionAvail().x - resetButtonSize * 2.0f, ImGui::GetFrameHeight()};

            ImGui::Dummy(size);

            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();

            const ImU32 bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
            const ImU32 border = ImGui::GetColorU32(ImGuiCol_Border);
            const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);

            // Draw frame
            draw->AddRectFilled(min, max, bg, 0.0f);
            draw->AddRect(min, max, border, 0.0f);

            // Text
            draw->AddText(
                ImVec2(min.x + 8.0f, min.y + 4.0f),
                textCol,
                value ? FormatTemp("{} ({})", assetName, typeName) : none.c_str()
            );

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(typeName.c_str())) {
                    if (const AssetID handle = *static_cast<const AssetID*>(payload->Data)) {
                        AssetRef<T> newAsset(handle);

                        if (newAsset) {
                            value = newAsset;
                            changed = true;
                        }
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

                for (auto& result : results) {
                    if (ImGui::Selectable(FormatTemp("{}", FS::Filename(result.Name)))) {
                        AssetRef<T> newAsset(result.Metadata->Handle);

                        if (newAsset) {
                            value = newAsset;
                            changed = true;
                        }

                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::EndGroup();

            ImGui::PopStyleVar(2);
            ImGui::PopItemWidth();

            ImGui::Columns(1);
            ImGui::PopID();

            ImGui::Spacing();

            return changed;
        }
    }
}
