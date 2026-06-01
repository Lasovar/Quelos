//
// Created by lasovar on 5/29/26.
//

#include "MaterialPropertiesPanel.h"

#include "EditorUI.h"
#include "MaterialCommands.h"

namespace QuelosEditor {
    void MaterialPropertiesPanel::OnImGuiRender(const ImGuiID dockspaceID, const ImGuiWindowClass& windowClass) const {
        if (!m_Material) {
            return;
        }

        Material& material = m_Material.Get();
        if (UI::Begin("Material Properties", dockspaceID, windowClass)) {
            AssetID shaderId = material.GetShader().GetAssetID();

            if (UI::EditAsset<GraphicsShader>("Shader", shaderId)) {
                material.SetShader(shaderId);
                m_UndoSystem.Push<SetMaterialShader>(m_Material, shaderId);
            }

            for (const MaterialPropertySpec& materialProperty : material.GetMaterialProperties()) {
                switch (materialProperty.Type) {
                case MaterialPropertyType::Float: {
                    static float startValue = 0.0f;
                    static bool startedEditing = false;

                    const float* value = material.GetProperty<float>(materialProperty.Offset);
                    if (!value) {
                        break;
                    }

                    float temp = *value;
                    if (UI::EditFloat(materialProperty.Name, temp)) {
                        if (!startedEditing) {
                            startedEditing = true;
                            startValue = *value;
                        }

                        material.SetProperty<float>(materialProperty.Offset, temp);
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_UndoSystem.Push<SetMaterialProperty<float>>(
                            m_Material,
                            materialProperty.Offset,
                            startValue,
                            temp
                        );

                        startedEditing = false;
                    }

                    break;
                }
                case MaterialPropertyType::Float2:
                case MaterialPropertyType::Float3:
                case MaterialPropertyType::Float4:
                case MaterialPropertyType::Color: {
                    static Color startValue;
                    static bool startedEditing = false;

                    const Color* value = material.GetProperty<Color>(materialProperty.Offset);
                    if (!value) {
                        break;
                    }

                    Color temp = *value;
                    if (UI::EditColor4(materialProperty.Name, temp)) {
                        if (!startedEditing) {
                            startedEditing = true;
                            startValue = *value;
                        }

                        material.SetProperty(materialProperty.Offset, temp);
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        m_UndoSystem.Push<SetMaterialProperty<Color>>(
                            m_Material,
                            materialProperty.Offset,
                            startValue,
                            temp
                        );

                        startedEditing = false;
                    }
                    break;
                }
                case MaterialPropertyType::Unknown:
                case MaterialPropertyType::Int:
                case MaterialPropertyType::Int2:
                case MaterialPropertyType::Int3:
                case MaterialPropertyType::Int4:
                case MaterialPropertyType::UInt:
                case MaterialPropertyType::Texture2D: {
                    const auto& value = material.GetProperty<AssetRef<Texture2D>>(materialProperty.Offset);
                    if (!value) {
                        break;
                    }

                    AssetID assetId = value->GetAssetID();
                    if (UI::EditAsset<Texture2D>(materialProperty.Name, assetId)) {
                        m_UndoSystem.Push<SetMaterialProperty<AssetRef<Texture2D>>>(
                            m_Material,
                            materialProperty.Offset,
                            value->GetAssetID(),
                            assetId
                        );
                    }
                    break;
                }
                case MaterialPropertyType::UInt2:
                case MaterialPropertyType::UInt3:
                case MaterialPropertyType::UInt4:
                    break;
                }
            }
        }
        ImGui::End();
    }
}
