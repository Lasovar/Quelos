//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "GpuBuffer.h"
#include "PipelineState.h"

namespace Quelos {
    enum class MaterialProperty {
        Unknown,
        Float, Float2, Float3, Float4,
        Color, // Attributed float4
        Int, Int2, Int3, Int4,
        UInt, UInt2, UInt3, UInt4
    };

    class QS_API GraphicsShader : public Asset {
    public:
        GraphicsShader(
            Buffer vertex,
            Buffer fragment,
            std::string name,
            const HashMap<std::string, MaterialProperty>& materialProperties
        );
        ~GraphicsShader() override;

        [[nodiscard]] const std::string& GetName() const { return m_Name; }

        void Recreate(Buffer vertex, Buffer fragment);

        [[nodiscard]] ShaderHandle GetVertexShaderHandle() const { return m_VertexShader; }
        [[nodiscard]] ShaderHandle GetFragmentShaderHandle() const { return m_FragmentShader; }

        void AddPipelineState(const PipelineStateHandle pipelineState) {
            m_PipelineStates.push_back(pipelineState);
        }

        void RemovePipelineState(const PipelineStateHandle pipelineStateHandle) {
            std::erase(m_PipelineStates, pipelineStateHandle);
        }

    private:
        std::string m_Name;

        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;

        Vec<PipelineStateHandle> m_PipelineStates;
        HashMap<std::string, MaterialProperty> m_MaterialProperties;

    public:
        [[nodiscard]] const AssetType& GetAssetType() const override { return GetStaticType(); }

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<GraphicsShader>();
            return s_AssetType;
        }
    };

    struct QS_API GraphicsShaderHandle : Handle<GraphicsShader> {
        GraphicsShaderHandle() = default;

        GraphicsShaderHandle(const Handle shaderHandle) {
            Value = shaderHandle.Value;
        }

        void Submit(uint32_t viewId) const;
    };
}
