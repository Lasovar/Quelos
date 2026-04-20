#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Utility/SlotMap.h"
#include "Quelos/Core/Buffer.h"

namespace Quelos {
    class Shader;

    struct QS_API ShaderHandle : Handle<Shader> {
        ShaderHandle() = default;
        ShaderHandle(const Handle shaderHandle) {
            Value = shaderHandle.Value;
        }

        void Submit(uint32_t viewId) const;
    };

    class QS_API Shader : public Asset {
    public:
        Shader(Buffer vertex, Buffer fragment, std::string name);
        ~Shader() override;

        const std::string& GetName() const { return m_Name; }
        ShaderHandle GetHandle() const { return m_ShaderHandle; }

        void Recreate(Buffer vertex, Buffer fragment) const;

    private:
        std::string m_Name;
        ShaderHandle m_ShaderHandle;

    public:
        const AssetType& GetAssetType() const override { return s_AssetType; }
        static const AssetType& GetStaticType() { return s_AssetType; }
    private:
        static AssetType s_AssetType;
    };
}
