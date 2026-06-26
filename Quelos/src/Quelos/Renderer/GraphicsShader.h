//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Quelos/Core/Buffer.h"
#include "variant"

#include "GpuBuffer.h"
#include "PipelineState.h"

namespace Quelos {
    enum class MaterialPropertyType : uint8_t {
        Unknown,
        Float, Float2, Float3, Float4,
        Color, // a float4 with a [Color] Attribute
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

    enum class PipelineOption : uint32_t {
        None,
        DepthEnable,
        DepthWriteEnable,
        CullMode
    };

    using PipelineOptionValue = std::variant<int32_t, std::string>;

    struct QS_API ShaderData {
        ShaderType Type = ShaderType::Unknown;
        std::string EntryPoint;
        BufferView Code;
        int32_t Order;
        Vec<Pair<PipelineOption, PipelineOptionValue>> PipelineOptions;
    };

    struct QS_API GraphicsShaderCreateInfo {
        std::string_view Name;
        HashMap<std::string, SmallVec<ShaderData, 2>> Passes;
        Span<const MaterialPropertySpec> MaterialProperties;
        uint64_t MaterialSize = 0;
    };

    struct QS_API GraphicsShaderPipelineData {
        int32_t Order = 0;
        ShaderHandle VertexShader;
        ShaderHandle FragmentShader;
        HashMap<PipelineOption, PipelineOptionValue> PipelineOptions;
    };

    struct QS_API GraphicsShaderPass {
        std::string Name;
        Vec<GraphicsShaderPipelineData> Pipelines;
    };

    class QS_API GraphicsShader : public Asset {
    public:
        explicit GraphicsShader(const GraphicsShaderCreateInfo& createInfo);

        ~GraphicsShader() override;

        [[nodiscard]] const std::string& GetName() const { return m_Name; }

        [[nodiscard]] const GraphicsShaderPass* GetShaderPass(std::string_view name) const {
            const auto it = std::ranges::find_if(
                m_Passes,
                [&name](const GraphicsShaderPass& pass) {
                    return pass.Name == name;
                }
            );

            if (it == m_Passes.end()) {
                return nullptr;
            }

            return &*it;
        }

        // Add a PiplineStateObject associated with this asset
        // If the asset is released, all PipelineStateObjects associated with it will be released
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

        Vec<GraphicsShaderPass> m_Passes;

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
