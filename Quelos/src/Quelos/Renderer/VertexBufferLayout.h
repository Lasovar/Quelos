#pragma once
#include <cstdint>

namespace Quelos {
    enum class ValueType : uint8_t {
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

    constexpr uint32_t ShaderDataTypeSize(const ValueType type) {
        switch (type) {
        case ValueType::Undefined: return 0;
        case ValueType::Float:  return 4;
        case ValueType::Float2: return 4 * 2;
        case ValueType::Float3: return 4 * 3;
        case ValueType::Float4: return 4 * 4;
        case ValueType::Float3x3:   return 4 * 3 * 3;
        case ValueType::Float4x4:   return 4 * 4 * 4;

        case ValueType::Int:  return 4;
        case ValueType::Int2: return 4 * 2;
        case ValueType::Int3: return 4 * 3;
        case ValueType::Int4: return 4 * 4;
        case ValueType::UInt: return 4;
        case ValueType::UInt16:  return 2;
        case ValueType::UInt10x3_A2: return 4;

        case ValueType::UNorm8x2:  return 1 * 2;
        case ValueType::UNorm8x4:  return 1 * 4;
        case ValueType::UNorm16x2: return 2 * 2;
        case ValueType::UNorm16x4: return 2 * 4;

        case ValueType::SNorm8x2:  return 1 * 2;
        case ValueType::SNorm8x4:  return 1 * 4;
        case ValueType::SNorm16x2: return 2 * 2;
        case ValueType::SNorm16x4: return 2 * 4;
        }

        return 0;
    }

    constexpr uint8_t ComponentCount(const ValueType type) {
        switch (type) {
        case ValueType::Int: return 1;
        case ValueType::Int2: return 2;
        case ValueType::Int3: return 3;
        case ValueType::Int4: return 4;
        case ValueType::UInt: return 1;
        case ValueType::UInt16:  return 1;
        case ValueType::UInt10x3_A2: return 4;

        case ValueType::Float:  return 1;
        case ValueType::Float2: return 2;
        case ValueType::Float3: return 3;
        case ValueType::Float4: return 4;

        case ValueType::Float3x3: return 9;
        case ValueType::Float4x4: return 16;

        case ValueType::UNorm8x2:  return 2;
        case ValueType::UNorm8x4:  return 4;
        case ValueType::UNorm16x2: return 2;
        case ValueType::UNorm16x4: return 4;

        default: return 4;
        }
    }

    constexpr bool IsNormalized(const ValueType type) {
        switch (type) {
        case ValueType::UNorm8x2:
        case ValueType::UNorm8x4:
        case ValueType::UNorm16x2:
        case ValueType::UNorm16x4:
        case ValueType::SNorm8x2:
        case ValueType::SNorm8x4:
        case ValueType::SNorm16x2:
        case ValueType::SNorm16x4:
            return true;
        default:
            return false;
        }
    }

    constexpr bool IsIntegerType(const ValueType type) {
        switch (type) {
        case ValueType::UNorm8x2:
        case ValueType::UNorm8x4:
        case ValueType::UNorm16x2:
        case ValueType::UNorm16x4:
        case ValueType::SNorm8x2:
        case ValueType::SNorm8x4:
        case ValueType::SNorm16x2:
        case ValueType::SNorm16x4:
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
        ValueType Type;
        VertexAttribute Attribute;
        uint16_t Offset;
    };

    constexpr uint32_t k_MaxElements = 18;

    struct QS_API VertexLayout {
        std::array<BufferElement, k_MaxElements> Elements{};
        uint8_t Count = 0;
        uint16_t Stride = 0;

        constexpr void Add(const VertexAttribute attr, const ValueType type) {
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
