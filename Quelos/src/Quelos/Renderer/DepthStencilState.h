//
// Created by lasovar on 5/23/26.
//

#pragma once

#include "GraphicsTypes.h"

namespace Quelos {
    enum class StencilOp : int8_t {
        /// Undefined operation.
        Undefined = 0,

        /// Keep the existing stencil data.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_KEEP/D3D12_STENCIL_OP_KEEP. OpenGL counterpart: GL_KEEP.
        Keep = 1,

        /// Set the stencil data to 0.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_ZERO/D3D12_STENCIL_OP_ZERO. OpenGL counterpart: GL_ZERO.
        Zero = 2,

        /// Set the stencil data to the reference value set by calling IDeviceContext::SetStencilRef().\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_REPLACE/D3D12_STENCIL_OP_REPLACE. OpenGL counterpart: GL_REPLACE.
        Replace = 3,

        /// Increment the current stencil value, and clamp to the maximum representable unsigned value.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_INCR_SAT/D3D12_STENCIL_OP_INCR_SAT. OpenGL counterpart: GL_INCR.
        IncrSat = 4,

        /// Decrement the current stencil value, and clamp to 0.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_DECR_SAT/D3D12_STENCIL_OP_DECR_SAT. OpenGL counterpart: GL_DECR.
        DecrSat = 5,

        /// Bitwise invert the current stencil buffer value.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_INVERT/D3D12_STENCIL_OP_INVERT. OpenGL counterpart: GL_INVERT.
        Invert = 6,

        /// Increment the current stencil value, and wrap the value to zero when incrementing
        /// the maximum representable unsigned value. \n
        /// Direct3D counterpart: D3D11_STENCIL_OP_INCR/D3D12_STENCIL_OP_INCR. OpenGL counterpart: GL_INCR_WRAP.
        IncrWrap = 7,

        /// Decrement the current stencil value, and wrap the value to the maximum representable
        /// unsigned value when decrementing a value of zero.\n
        /// Direct3D counterpart: D3D11_STENCIL_OP_DECR/D3D12_STENCIL_OP_DECR. OpenGL counterpart: GL_DECR_WRAP.
        DecrWrap = 8,

        /// Helper value that stores the total number of stencil operations in the enumeration.
        Count
    };

    struct StencilOpSpec {
        /// The stencil operation to perform when stencil testing fails.
        /// Default value: StencilOp::Keep
        StencilOp          StencilFailOp = StencilOp::Keep;

        /// The stencil operation to perform when stencil testing passes and depth testing fails.
        /// Default value: StencilOp::Keep
        StencilOp          StencilDepthFailOp = StencilOp::Keep;

        /// The stencil operation to perform when stencil testing and depth testing both pass.
        /// Default value: StencilOp::Keep
        StencilOp          StencilPassOp = StencilOp::Keep;

        /// A function that compares stencil data against existing stencil data.
        /// Default value: ComparisonFunc::Always
        ComparisonFunc StencilFunc = ComparisonFunc::Always;
    };

    /// Depth stencil state description

    /// [D3D11_DEPTH_STENCIL_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476110(v=vs.85).aspx
    /// [D3D12_DEPTH_STENCIL_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770356(v=vs.85).aspx
    /// This structure describes the depth stencil state and is part of the GraphicsPipelineDesc.
    /// The structure generally mirrors [D3D11_DEPTH_STENCIL_DESC][]/[D3D12_DEPTH_STENCIL_DESC][]
    /// structure.
    struct DepthStencilStateSpec {
        /// Enable depth-stencil operations. When it is set to False,
        /// depth test always passes, depth writes are disabled,
        /// and no stencil operations are performed. Default value: True.
        bool DepthEnable = true;

        /// Enable or disable writes to a depth buffer. Default value: True.
        bool DepthWriteEnable = true;

        /// A function that compares depth data against existing depth data.
        /// See Diligent::COMPARISON_FUNCTION for details.
        /// Default value: ComparisonFunc::Less
        ComparisonFunc DepthFunc = ComparisonFunc::Less;

        /// Enable stencil operations. Default value: False.
        bool StencilEnable = false;

        /// Identify which bits of the depth-stencil buffer are accessed when reading stencil data.
        /// Default value: 0xFF.
        uint8_t StencilReadMask = 0xFF;

        /// Identify which bits of the depth-stencil buffer are accessed when writing stencil data.
        /// Default value: 0xFF.
        uint8_t StencilWriteMask = 0xFF;

        /// Identify stencil operations for the front-facing triangles, see Diligent::StencilOpDesc.
        StencilOpSpec FrontFace;

        /// Identify stencil operations for the back-facing triangles, see Diligent::StencilOpDesc.
        StencilOpSpec BackFace;
    };
}
