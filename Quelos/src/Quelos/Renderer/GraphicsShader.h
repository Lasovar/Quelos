//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "GpuBuffer.h"
#include "PipelineState.h"

namespace Quelos {
    class QS_API GraphicsShader : public Asset {
    public:
        GraphicsShader(Buffer vertex, Buffer fragment, std::string name);
        ~GraphicsShader() override;

        const std::string& GetName() const { return m_Name; }

        void Recreate(Buffer vertex, Buffer fragment);

        ShaderHandle GetVertexShaderHandle() const { return m_VertexShader; }
        ShaderHandle GetFragmentShaderHandle() const { return m_FragmentShader; }

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

    public:
        const AssetType& GetAssetType() const override { return GetStaticType(); }

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
