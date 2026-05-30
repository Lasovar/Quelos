//
// Created by lasovar on 5/29/26.
//

#include "MaterialPropertiesPanel.h"

#include "EditorUI.h"

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
            }

            for (const MaterialPropertySpec& materialProperty : material.GetMaterialProperties()) {
                switch (materialProperty.Type) {
                case MaterialPropertyType::Float: {
                    auto value = material.GetProperty<float>(materialProperty.Offset, materialProperty.Size);
                    if (UI::EditFloat(materialProperty.Name, value)) {
                        material.SetProperty(materialProperty.Offset, materialProperty.Size, &value);
                    }

                    break;
                }
                case MaterialPropertyType::Float2:
                case MaterialPropertyType::Float3:
                case MaterialPropertyType::Float4:
                case MaterialPropertyType::Color: {
                    auto value = material.GetProperty<Color>(materialProperty.Offset, materialProperty.Size);
                    if (UI::EditColor4(materialProperty.Name, value)) {
                        material.SetProperty(materialProperty.Offset, materialProperty.Size, &value);
                    }
                    break;
                }
                case MaterialPropertyType::Unknown:
                case MaterialPropertyType::Int:
                case MaterialPropertyType::Int2:
                case MaterialPropertyType::Int3:
                case MaterialPropertyType::Int4:
                case MaterialPropertyType::UInt:
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
