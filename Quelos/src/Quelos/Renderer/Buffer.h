//
// Created by lasovar on 5/24/26.
//

#pragma once

#include "Quelos/Core/API.h"
#include "GraphicsTypes.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class BufferMode : uint8_t {
        /// Undefined mode.
        Undefined = 0,

        /// Formatted buffer. Access to the buffer will use format conversion operations.
        /// In this mode, ElementByteStride member of BufferDesc defines the buffer element size.
        /// Buffer views can use different formats, but the format size must match ElementByteStride.
        Formatted,

        /// Structured buffer.
        /// In this mode, ElementByteStride member of BufferDesc defines the structure stride.
        Structured,

        /// Raw buffer.
        /// In this mode, the buffer is accessed as raw bytes. Formatted views of a raw
        /// buffer can also be created similar to formatted buffer. If formatted views
        /// are to be created, the ElementByteStride member of BufferDesc must specify the
        /// size of the format.
        Raw,

        /// Helper value storing the total number of modes in the enumeration.
        Count
    };

    enum class MiscBufferFlags : uint8_t {
        /// No special flags are set.
        None = 0,

        /// For a sparse buffer, allow binding the same memory region in different buffer ranges
        /// or in different sparse buffers.
        SparseAliasing = 1u << 0,
    };

    struct QS_API GPUBufferSpec {
        std::string_view Name;
        uint64_t Size = 0;
        BindFlags BindFlags = Bind::None;
        Usage Usage = Usage::Default;
        CpuAccessFlags CpuAccessFlags = CpuAccess::None;
        BufferMode Mode = BufferMode::Undefined;
        MiscBufferFlags MiscFlags = MiscBufferFlags::None;
        uint32_t ElementByteStride = 0;
        uint64_t ImmediateContextMask = 1;
    };

    class GPUBuffer;

    struct GPUBufferHandle : Handle<GPUBuffer> {
        GPUBufferHandle() = default;
        GPUBufferHandle(const Handle handle) : Handle(handle) {}
    };
}
