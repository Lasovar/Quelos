#pragma once
#include <cstdint>

namespace Quelos {
    enum class ShaderDataType : uint8_t {
        Undefined,
        Float, Float2, Float3, Float4,
        Float3x3, Float4x4,

        Int, Int2, Int3, Int4,
        UInt,
        UInt16,
        UInt10x3_A2,

        // normalized
        UNorm8x2,
        UNorm8x4,
        UNorm16x2,
        UNorm16x4,

        SNorm8x2,
        SNorm8x4,
        SNorm16x2,
        SNorm16x4,
    };

    constexpr uint32_t ShaderDataTypeSize(const ShaderDataType type) {
        switch (type) {
        case ShaderDataType::Undefined: return 0;
        case ShaderDataType::Float:  return 4;
        case ShaderDataType::Float2: return 4 * 2;
        case ShaderDataType::Float3: return 4 * 3;
        case ShaderDataType::Float4: return 4 * 4;
        case ShaderDataType::Float3x3:   return 4 * 3 * 3;
        case ShaderDataType::Float4x4:   return 4 * 4 * 4;

        case ShaderDataType::Int:  return 4;
        case ShaderDataType::Int2: return 4 * 2;
        case ShaderDataType::Int3: return 4 * 3;
        case ShaderDataType::Int4: return 4 * 4;
        case ShaderDataType::UInt: return 4;
        case ShaderDataType::UInt16:  return 2;
        case ShaderDataType::UInt10x3_A2: return 4;

        case ShaderDataType::UNorm8x2:  return 1 * 2;
        case ShaderDataType::UNorm8x4:  return 1 * 4;
        case ShaderDataType::UNorm16x2: return 2 * 2;
        case ShaderDataType::UNorm16x4: return 2 * 4;

        case ShaderDataType::SNorm8x2:  return 1 * 2;
        case ShaderDataType::SNorm8x4:  return 1 * 4;
        case ShaderDataType::SNorm16x2: return 2 * 2;
        case ShaderDataType::SNorm16x4: return 2 * 4;
        }

        return 0;
    }

    constexpr uint8_t ComponentCount(const ShaderDataType type) {
        switch (type) {
        case ShaderDataType::Int: return 1;
        case ShaderDataType::Int2: return 2;
        case ShaderDataType::Int3: return 3;
        case ShaderDataType::Int4: return 4;
        case ShaderDataType::UInt: return 1;
        case ShaderDataType::UInt16:  return 1;
        case ShaderDataType::UInt10x3_A2: return 4;

        case ShaderDataType::Float:  return 1;
        case ShaderDataType::Float2: return 2;
        case ShaderDataType::Float3: return 3;
        case ShaderDataType::Float4: return 4;

        case ShaderDataType::Float3x3: return 9;
        case ShaderDataType::Float4x4: return 16;

        case ShaderDataType::UNorm8x2:  return 2;
        case ShaderDataType::UNorm8x4:  return 4;
        case ShaderDataType::UNorm16x2: return 2;
        case ShaderDataType::UNorm16x4: return 4;

        default: return 4;
        }
    }

    constexpr bool IsNormalized(const ShaderDataType type) {
        switch (type) {
        case ShaderDataType::UNorm8x2:
        case ShaderDataType::UNorm8x4:
        case ShaderDataType::UNorm16x2:
        case ShaderDataType::UNorm16x4:
        case ShaderDataType::SNorm8x2:
        case ShaderDataType::SNorm8x4:
        case ShaderDataType::SNorm16x2:
        case ShaderDataType::SNorm16x4:
            return true;
        default:
            return false;
        }
    }

    constexpr bool IsIntegerType(const ShaderDataType type) {
        switch (type) {
        case ShaderDataType::UNorm8x2:
        case ShaderDataType::UNorm8x4:
        case ShaderDataType::UNorm16x2:
        case ShaderDataType::UNorm16x4:
        case ShaderDataType::SNorm8x2:
        case ShaderDataType::SNorm8x4:
        case ShaderDataType::SNorm16x2:
        case ShaderDataType::SNorm16x4:
            return true;
        default:
            return false;
        }
    }

    enum class QS_API VertexAttribute : uint8_t {
        Position,
        Normal,
        Tangent,
        Bitangent,

        Color0,
        Color1,
        Color2,
        Color3,

        Indices,
        Weight,

        TexCoord0,
        TexCoord1,
        TexCoord2,
        TexCoord3,
        TexCoord4,
        TexCoord5,
        TexCoord6,
        TexCoord7,

        Count
    };

    struct QS_API BufferElement {
        ShaderDataType Type;
        VertexAttribute Attribute;
        uint16_t Offset;
    };

    constexpr uint32_t k_MaxElements = 18;

    struct QS_API VertexLayout {
        std::array<BufferElement, k_MaxElements> Elements{};
        uint8_t Count = 0;
        uint16_t Stride = 0;

        constexpr void Add(const VertexAttribute attr, const ShaderDataType type) {
            BufferElement& element = Elements[Count];
            element.Type = type;
            element.Attribute = attr;
            element.Offset = Stride;

            Stride += ShaderDataTypeSize(type);
            Count++;
        }

        constexpr const BufferElement& operator[](const size_t i) const {
            return Elements[i];
        }
    };
}
