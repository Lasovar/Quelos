//
// Created by lasovar on 5/23/26.
//

#pragma once

#include <array>

#include "VertexBufferLayout.h"
#include "Quelos/Core/Assert.h"

namespace Quelos {
    enum class InputElementFrequency : uint8_t {
        /// Frequency is undefined.
        Undefined = 0,

        /// Input data is per-vertex data.
        PerVertex,

        /// Input data is per-instance data.
        PerInstance,

        /// Helper value that stores the total number of frequencies in the enumeration.
        Count
    };

    struct LayoutElement {
        /// Input index of the element that is specified in the vertex shader.

        /// In Direct3D11 and Direct3D12 backends this is the semantic index.
        uint32_t InputIndex = 0;

        /// Buffer slot index that this element is read from.
        uint32_t BufferSlot = 0;

        /// Type of the element components, see Quelos::ShaderDataType for details.
        ValueType Type = ValueType::Float;

        /// Indicates if the value should be normalized.

        /// For signed and unsigned integer value types
        /// (`VT_INT8`, `VT_INT16`, `VT_INT32`, `VT_UINT8`, `VT_UINT16`, `VT_UINT32`)
        /// indicates if the value should be normalized to [-1,+1] or
        /// [0, 1] range respectively.
        ///
        /// For floating point types (`VT_FLOAT16` and `VT_FLOAT32`), this member is ignored.
        bool IsNormalized = true;

        /// Relative offset, in bytes, to the element bits.

        /// If this value is set to `LAYOUT_ELEMENT_AUTO_OFFSET` (default value), the offset will
        /// be computed automatically by placing the element right after the previous one.
        uint32_t RelativeOffset;

        /// Stride, in bytes, between two elements, for this buffer slot.

        /// If this value is set to `LAYOUT_ELEMENT_AUTO_STRIDE`, the stride will be
        /// computed automatically assuming that all elements in the same buffer slot are
        /// packed one after another. If the buffer slot contains multiple layout elements,
        /// they all must specify the same stride or use LAYOUT_ELEMENT_AUTO_STRIDE value.
        uint32_t Stride;

        /// Frequency of the input data, see Diligent::INPUT_ELEMENT_FREQUENCY for details.
        InputElementFrequency Frequency = InputElementFrequency::PerVertex;

        /// The number of instances to draw using the same per-instance data before advancing
        /// in the buffer by one element.
        uint32_t InstanceDataStepRate = 1;

        LayoutElement() = default;
        LayoutElement(
            const uint32_t inputIndex,
            const uint32_t bufferSlot,
            const ValueType type,
            const InputElementFrequency frequency = InputElementFrequency::PerVertex,
            const uint32_t stepRate = 1
        )
            : InputIndex(inputIndex),
              BufferSlot(bufferSlot),
              Type(type),
              RelativeOffset(0),
              Stride(0),
              Frequency(frequency),
              InstanceDataStepRate(stepRate) {}
    };

    template <std::size_t N>
    struct LayoutElementBuilder {
        std::array<LayoutElement, N> Elements{};
        uint32_t Count = 0;
        uint32_t Stride = 0;

        constexpr LayoutElementBuilder() = default;

        template<typename... T>
        constexpr LayoutElementBuilder(T&&... entries) {
            (Add(entries.InputIndex, entries.Type), ...);
        }

        constexpr LayoutElementBuilder& Add(
            const uint32_t inputIndex,
            const ValueType type,
            const uint32_t bufferSlot = 0,
            const InputElementFrequency frequency = InputElementFrequency::PerVertex,
            const uint32_t stepRate = 1
        ) {
            QS_CORE_ASSERT(Count < N);

            auto& element = Elements[Count];

            element.InputIndex = inputIndex;
            element.BufferSlot = bufferSlot;
            element.Type = type;
            element.IsNormalized = IsNormalized(type);
            element.RelativeOffset = Stride;
            element.Stride = 0; // filled later
            element.Frequency = frequency;
            element.InstanceDataStepRate = stepRate;

            Stride += ShaderDataTypeSize(type);

            Count++;

            UpdateStrides(bufferSlot);

            return *this;
        }

        constexpr const LayoutElement& operator[](const std::size_t i) const {
            return Elements[i];
        }

        operator Span32<const LayoutElement>() const {
            return Span32<const LayoutElement>(Elements.data(), Count);
        }

    private:
        constexpr void UpdateStrides(const uint32_t bufferSlot) {
            uint32_t stride = 0;

            for (uint32_t i = 0; i < Count; i++) {
                const auto& element = Elements[i];

                if (element.BufferSlot != bufferSlot) {
                    continue;
                }

                stride += ShaderDataTypeSize(element.Type);
            }

            for (uint32_t i = 0; i < Count; i++) {
                LayoutElement& element = Elements[i];

                if (element.BufferSlot != bufferSlot) {
                    continue;
                }

                element.Stride = stride;
            }
        }
    };

    struct InputLayoutSpec {
        Span32<const LayoutElement> LayoutElements;
    };
}
