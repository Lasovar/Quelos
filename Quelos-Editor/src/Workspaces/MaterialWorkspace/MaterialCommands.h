//
// Created by lasovar on 6/1/26.
//

#pragma once
#include "Quelos/AssetManager/AssetRef.h"
#include "Quelos/Renderer/Material.h"

namespace QuelosEditor {
    using namespace Quelos;

    template <typename TPropertyValue>
    struct SetMaterialProperty {
        AssetRef<Material> MaterialAsset;
        uint64_t PropertyOffset = 0;

        TPropertyValue Before;
        TPropertyValue After;

        SetMaterialProperty(
            const AssetRef<Material>& material,
            const uint64_t offset,
            TPropertyValue beforeValue,
            TPropertyValue newValue
        ) : MaterialAsset(material), PropertyOffset(offset), Before(beforeValue), After(newValue) { }

        void Apply() const {
            if (!MaterialAsset) {
                return;
            }

            MaterialAsset.Get().template SetProperty<TPropertyValue>(PropertyOffset, After);
        }

        void Revert() const {
            if (!MaterialAsset) {
                return;
            }

            MaterialAsset.Get().template SetProperty<TPropertyValue>(PropertyOffset, Before);
        }
    };

    template <>
    struct SetMaterialProperty<AssetRef<Texture2D>> {
        AssetRef<Material> MaterialAsset;
        uint64_t PropertyOffset = 0;

        AssetID Before;
        AssetID After;

        SetMaterialProperty(
            const AssetRef<Material>& material,
            const uint64_t offset,
            const AssetID beforeValue,
            const AssetID newValue
        ) : MaterialAsset(material), PropertyOffset(offset), Before(beforeValue), After(newValue) { }

        void Apply() const {
            if (!MaterialAsset) {
                return;
            }

            MaterialAsset.Get().SetProperty(PropertyOffset, AssetRef<Texture2D>(After));
        }

        void Revert() const {
            if (!MaterialAsset) {
                return;
            }

            MaterialAsset.Get().SetProperty(PropertyOffset, AssetRef<Texture2D>(Before));
        }
    };

    struct SetMaterialShader {
        AssetRef<Material> MaterialAsset;

        AssetID Before;
        Vec<MaterialPropertyValue> BeforeValues;
        AssetID After;

        SetMaterialShader(const AssetRef<Material>& materialAsset, const AssetID newShader)
            : MaterialAsset(materialAsset), After(newShader)
        {
            const Material& material = MaterialAsset.Get();
            Before = material.GetShader().GetAssetID();
            BeforeValues = material.GetMaterialPropertyValues();
        }

        void Apply() const {
            if (!MaterialAsset) {
                return;
            }

            Material& material = MaterialAsset.Get();
            material.SetShader(After);
        }

        void Revert() const {
            if (!MaterialAsset) {
                return;
            }

            Material& material = MaterialAsset.Get();
            material.SetShader(Before);

            const Vec<MaterialPropertySpec>& materialProperties = material.GetMaterialProperties();
            for (uint32_t i = 0; i < materialProperties.size(); i++) {
                material.SetProperty(materialProperties[i].Offset, BeforeValues[i]);
            }
        }
    };
}
