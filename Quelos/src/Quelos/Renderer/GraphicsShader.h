//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Buffer.h"
#include "PipelineState.h"

namespace Quelos {
    class QS_API GraphicsShader : public Asset {
    public:
        GraphicsShader(Buffer vertex, Buffer fragment, std::string name);
        ~GraphicsShader() override;

        const std::string& GetName() const { return m_Name; }

        bool IsPSOCreated() const { return m_PipelineStateHandle.IsValid(); }
        void CreatePSO(RenderPassHandle renderPassHandle, GPUBufferHandle globalBufferHandle);
        PipelineStateHandle GetPipelineStateHandle() const { return m_PipelineStateHandle; }

        void Recreate(Buffer vertex, Buffer fragment);

    private:
        std::string m_Name;
        PipelineStateHandle m_PipelineStateHandle;

        Buffer m_VertexBlob;
        Buffer m_FragmentBlob;

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
