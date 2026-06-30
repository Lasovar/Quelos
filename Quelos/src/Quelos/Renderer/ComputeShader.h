//
// Created by lasovar on 6/28/26.
//

#pragma once

#include "Shader.h"
#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
    struct QS_API ComputeShaderCreateInfo {
        std::string Name;
        std::string EntryPoint;
        BufferView Code;
        int32_t Order;
        Array<uint32_t, 3> ThreadGroupSize;
    };

    class QS_API ComputeShader : public Asset {
    public:
        explicit ComputeShader(const ComputeShaderCreateInfo& createInfo);

        [[nodiscard]] ShaderHandle GetShader() const { return m_Shader; }

        ~ComputeShader() override;
        const Array<uint32_t, 3>& GetThreadGroupSize() const { return m_ThreadGroupSize; }

    private:
        std::string m_Name;
        ShaderHandle m_Shader;
        Array<uint32_t, 3> m_ThreadGroupSize;

    public:
        [[nodiscard]] const AssetType& GetAssetType() const override { return GetStaticType(); }

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<ComputeShader>();
            return s_AssetType;
        }
    };
}
