//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Quelos/Core/Buffer.h"

#include "GpuBuffer.h"
#include "PipelineState.h"

namespace Quelos {
    enum class MaterialPropertyType : uint8_t {
        Unknown,
        Float, Float2, Float3, Float4,
        Color, // Attributed float4
        Int, Int2, Int3, Int4,
        Texture2D,
        UInt,
        UInt2, UInt3, UInt4
    };

    constexpr uint8_t GetMaterialPropertyTypeSize(const MaterialPropertyType materialProperty) {
        switch (materialProperty) {
        case MaterialPropertyType::Unknown: return 0;
        case MaterialPropertyType::Float:   return 4;
        case MaterialPropertyType::Float2:  return 4 * 2;
        case MaterialPropertyType::Float3:  return 4 * 3;

        case MaterialPropertyType::Color:
        case MaterialPropertyType::Float4:  return 4 * 4;

        case MaterialPropertyType::Int:     return 4;
        case MaterialPropertyType::Int2:    return 4 * 2;
        case MaterialPropertyType::Int3:    return 4 * 3;
        case MaterialPropertyType::Int4:    return 4 * 4;

        case MaterialPropertyType::Texture2D:
        case MaterialPropertyType::UInt:    return 4;

        case MaterialPropertyType::UInt2:   return 4 * 2;
        case MaterialPropertyType::UInt3:   return 4 * 3;
        case MaterialPropertyType::UInt4:   return 4 * 4;
        }

        return 0;
    }

    struct QS_API MaterialPropertySpec {
        std::string Name;
        MaterialPropertyType Type = MaterialPropertyType::Unknown;
        uint64_t Size = 0;
        uint64_t Offset = 0;
    };

    class QS_API GraphicsShader : public Asset {
    public:
        GraphicsShader(
            Buffer vertex,
            Buffer fragment,
            std::string name,
            const Vec<MaterialPropertySpec>& materialProperties,
            uint64_t materialSize
        );

        ~GraphicsShader() override;

        [[nodiscard]] const std::string& GetName() const { return m_Name; }

        [[nodiscard]] ShaderHandle GetVertexShaderHandle() const { return m_VertexShader; }
        [[nodiscard]] ShaderHandle GetFragmentShaderHandle() const { return m_FragmentShader; }

        void AddPipelineState(const PipelineStateHandle pipelineState) {
            m_PipelineStates.push_back(pipelineState);
        }

        void RemovePipelineState(const PipelineStateHandle pipelineStateHandle) {
            std::erase(m_PipelineStates, pipelineStateHandle);
        }

        [[nodiscard]] const Vec<MaterialPropertySpec>& GetMaterialProperties() const { return m_MaterialProperties; }
        [[nodiscard]] uint64_t GetMaterialSize() const { return m_MaterialSize; }

    private:
        std::string m_Name;

        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;

        Vec<PipelineStateHandle> m_PipelineStates;
        Vec<MaterialPropertySpec> m_MaterialProperties;

        uint64_t m_MaterialSize;

    public:
        [[nodiscard]] const AssetType& GetAssetType() const override { return GetStaticType(); }

        static const AssetType& GetStaticType() {
            static AssetType s_AssetType = Quelos::GetAssetType<GraphicsShader>();
            return s_AssetType;
        }
    };
}
