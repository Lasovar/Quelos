//
// Created by lasovar on 6/20/26.
//

// COPIED FROM DILIGENT ENGINE

#pragma once

#include "GraphicsTypes.h"

namespace Quelos {
    constexpr uint32_t k_MaxRenderTargets = 8;

    enum class BlendFactor : int8_t {
        /// Undefined blend factor
        Undefined = 0,

        /// The blend factor is zero.\n
        /// Direct3D counterpart: D3D11_BLEND_ZERO/D3D12_BLEND_ZERO. OpenGL counterpart: GL_ZERO.
        Zero,

        /// The blend factor is one.\n
        /// Direct3D counterpart: D3D11_BLEND_ONE/D3D12_BLEND_ONE. OpenGL counterpart: GL_ONE.
        One,

        /// The blend factor is RGB data from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_SRC_COLOR/D3D12_BLEND_SRC_COLOR. OpenGL counterpart: GL_SRC_COLOR.
        SrcColor,

        /// The blend factor is 1-RGB, where RGB is the data from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_SRC_COLOR/D3D12_BLEND_INV_SRC_COLOR. OpenGL counterpart: GL_ONE_MINUS_SRC_COLOR.
        InvSrcColor,

        /// The blend factor is alpha (A) data from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_SRC_ALPHA/D3D12_BLEND_SRC_ALPHA. OpenGL counterpart: GL_SRC_ALPHA.
        SrcAlpha,

        /// The blend factor is 1-A, where A is alpha data from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_SRC_ALPHA/D3D12_BLEND_INV_SRC_ALPHA. OpenGL counterpart: GL_ONE_MINUS_SRC_ALPHA.
        InvSrcAlpha,

        /// The blend factor is alpha (A) data from a render target.\n
        /// Direct3D counterpart: D3D11_BLEND_DEST_ALPHA/D3D12_BLEND_DEST_ALPHA. OpenGL counterpart: GL_DST_ALPHA.
        DestAlpha,

        /// The blend factor is 1-A, where A is alpha data from a render target.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_DEST_ALPHA/D3D12_BLEND_INV_DEST_ALPHA. OpenGL counterpart: GL_ONE_MINUS_DST_ALPHA.
        InvDestAlpha,

        /// The blend factor is RGB data from a render target.\n
        /// Direct3D counterpart: D3D11_BLEND_DEST_COLOR/D3D12_BLEND_DEST_COLOR. OpenGL counterpart: GL_DST_COLOR.
        DestColor,

        /// The blend factor is 1-RGB, where RGB is the data from a render target.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_DEST_COLOR/D3D12_BLEND_INV_DEST_COLOR. OpenGL counterpart: GL_ONE_MINUS_DST_COLOR.
        InvDestColor,

        /// The blend factor is (f,f,f,1), where f = min(As, 1-Ad),
        /// As is alpha data from a pixel shader, and Ad is alpha data from a render target.\n
        /// Direct3D counterpart: D3D11_BLEND_SRC_ALPHA_SAT/D3D12_BLEND_SRC_ALPHA_SAT. OpenGL counterpart: GL_SRC_ALPHA_SATURATE.
        SrcAlphaSat,

        /// The blend factor is the constant blend factor set with IDeviceContext::SetBlendFactors().\n
        /// Direct3D counterpart: D3D11_BLEND_BLEND_FACTOR/D3D12_BLEND_BLEND_FACTOR. OpenGL counterpart: GL_CONSTANT_COLOR.
        ConstBlendFactor,

        /// The blend factor is one minus constant blend factor set with IDeviceContext::SetBlendFactors().\n
        /// Direct3D counterpart: D3D11_BLEND_INV_BLEND_FACTOR/D3D12_BLEND_INV_BLEND_FACTOR. OpenGL counterpart: GL_ONE_MINUS_CONSTANT_COLOR.
        InvConstBlendFactor,

        /// The blend factor is the second RGB data output from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_SRC1_COLOR/D3D12_BLEND_SRC1_COLOR. OpenGL counterpart: GL_SRC1_COLOR.
        Src1Color,

        /// The blend factor is 1-RGB, where RGB is the second RGB data output from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_SRC1_COLOR/D3D12_BLEND_INV_SRC1_COLOR. OpenGL counterpart: GL_ONE_MINUS_SRC1_COLOR.
        InvSrc1Color,

        /// The blend factor is the second alpha (A) data output from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_SRC1_ALPHA/D3D12_BLEND_SRC1_ALPHA. OpenGL counterpart: GL_SRC1_ALPHA.
        Src1Alpha,

        /// The blend factor is 1-A, where A is the second alpha data output from a pixel shader.\n
        /// Direct3D counterpart: D3D11_BLEND_INV_SRC1_ALPHA/D3D12_BLEND_INV_SRC1_ALPHA. OpenGL counterpart: GL_ONE_MINUS_SRC1_ALPHA.
        InvSrc1Alpha,

        /// Helper value that stores the total number of blend factors in the enumeration.
        Count
    };

    /// Blending operation

    /// [D3D11_BLEND_OP]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476088(v=vs.85).aspx
    /// [D3D12_BLEND_OP]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770340(v=vs.85).aspx
    /// [glBlendEquationSeparate]: https://www.opengl.org/wiki/GLAPI/glBlendEquationSeparate
    /// This enumeration describes blending operation for RGB or Alpha channels and generally mirrors
    /// [D3D11_BLEND_OP][] and [D3D12_BLEND_OP][] enums. It is used by RenderTargetBlendDesc structure to define RGB and Alpha
    /// blending operations
    /// \sa [D3D11_BLEND_OP on MSDN][D3D11_BLEND_OP], [D3D12_BLEND_OP on MSDN][D3D12_BLEND_OP], [glBlendEquationSeparate on OpenGL.org][glBlendEquationSeparate]
    enum class BlendOperation : int8_t {
        /// Undefined blend operation
        Undefined = 0,

        /// Add source and destination color components.\n
        /// Direct3D counterpart: D3D11_BLEND_OP_ADD/D3D12_BLEND_OP_ADD. OpenGL counterpart: GL_FUNC_ADD.
        Add,

        /// Subtract destination color components from source color components.\n
        /// Direct3D counterpart: D3D11_BLEND_OP_SUBTRACT/D3D12_BLEND_OP_SUBTRACT. OpenGL counterpart: GL_FUNC_SUBTRACT.
        Subtract,

        /// Subtract source color components from destination color components.\n
        /// Direct3D counterpart: D3D11_BLEND_OP_REV_SUBTRACT/D3D12_BLEND_OP_REV_SUBTRACT. OpenGL counterpart: GL_FUNC_REVERSE_SUBTRACT.
        RevSubtract,

        /// Compute the minimum of source and destination color components.\n
        /// Direct3D counterpart: D3D11_BLEND_OP_MIN/D3D12_BLEND_OP_MIN. OpenGL counterpart: GL_MIN.
        Min,

        /// Compute the maximum of source and destination color components.\n
        /// Direct3D counterpart: D3D11_BLEND_OP_MAX/D3D12_BLEND_OP_MAX. OpenGL counterpart: GL_MAX.
        Max,

        /// Helper value that stores the total number of blend operations in the enumeration.
        Count
    };

    enum class ColorMask : uint8_t {
        /// Do not store any components.
        None = 0u,

        /// Allow data to be stored in the red component.
        Red = 1u << 0u,

        /// Allow data to be stored in the green component.
        Green = 1u << 1u,

        /// Allow data to be stored in the blue component.
        Blue = 1u << 2u,

        /// Allow data to be stored in the alpha component.
        Alpha = 1u << 3u,

        /// Allow data to be stored in all RGB components.
        RBG = Red | Green | Blue,

        /// Allow data to be stored in all components.
        All = RBG | Alpha
    };

    enum class LogicOperation : int8_t {
        /// Clear the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_CLEAR.
        Clear = 0,

        /// Set the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_SET.
        Set,

        /// Copy the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_COPY.
        Copy,

        /// Perform an inverted-copy of the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_COPY_INVERTED.
        CopyInverted,

        /// No operation is performed on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_NOOP.
        Noop,

        /// Invert the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_INVERT.
        Invert,

        /// Perform a logical AND operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_AND.
        And,

        /// Perform a logical NAND operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_NAND.
        Nand,

        /// Perform a logical OR operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_OR.
        Or,

        /// Perform a logical NOR operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_NOR.
        Nor,

        /// Perform a logical XOR operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_XOR.
        Xor,

        /// Perform a logical equal operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_EQUIV.
        Equiv,

        /// Perform a logical AND and reverse operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_AND_REVERSE.
        AndReverse,

        /// Perform a logical AND and invert operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_AND_INVERTED.
        AndInverted,

        /// Perform a logical OR and reverse operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_OR_REVERSE.
        OrReverse,

        /// Perform a logical OR and invert operation on the render target.\n
        /// Direct3D12 counterpart: D3D12_LOGIC_OP_OR_INVERTED.
        OrInverted,

        /// Helper value that stores the total number of logical operations in the enumeration.
        Count
    };

    struct RenderTargetBlendSpec {
        /// Enable or disable blending for this render target. Default value: False.
        bool BlendEnable = false;

        /// Enable or disable a logical operation for this render target. Default value: False.
        bool LogicOperationEnable = false;

        /// Specifies the blend factor to apply to the RGB value output from the pixel shader
        /// Default value: BlendFactor::One.
        BlendFactor SrcBlend = BlendFactor::One;

        /// Specifies the blend factor to apply to the RGB value in the render target
        /// Default value: BlendFactor::Zero.
        BlendFactor DestBlend = BlendFactor::Zero;

        /// Defines how to combine the source and destination RGB values
        /// after applying the SrcBlend and DestBlend factors.
        /// Default value: BlendOperation::Add.
        BlendOperation BlendOp = BlendOperation::Add;

        /// Specifies the blend factor to apply to the alpha value output from the pixel shader.
        /// Blend factors that end in _COLOR are not allowed.
        /// Default value: BlendFactor::One.
        BlendFactor SrcBlendAlpha = BlendFactor::One;

        /// Specifies the blend factor to apply to the alpha value in the render target.
        /// Blend factors that end in _COLOR are not allowed.
        /// Default value: BlendFactor::Zero.
        BlendFactor DestBlendAlpha = BlendFactor::Zero;

        /// Defines how to combine the source and destination alpha values
        /// after applying the SrcBlendAlpha and DestBlendAlpha factors.
        /// Default value: BlendOperation::Add.
        BlendOperation BlendOpAlpha = BlendOperation::Add;

        /// Defines logical operation for the render target.
        /// Default value: LogicOperation::Noop.
        LogicOperation LogicOp = LogicOperation::Noop;

        /// Render target write mask.
        /// Default value: ColorMask::All.
        ColorMask RenderTargetWriteMask = ColorMask::All;
    };


    struct BlendStateSpec {
        /// Specifies whether to use alpha-to-coverage as a multisampling technique
        /// when setting a pixel to a render target. Default value: False.
        bool AlphaToCoverageEnable = false;

        /// Specifies whether to enable independent blending in simultaneous render targets.
        /// If set to False, only RenderTargets[0] is used. Default value: False.
        bool IndependentBlendEnable = false;

        /// An array of RenderTargetBlendSpec structures that describe the blend
        /// states for render targets
        RenderTargetBlendSpec RenderTargets[k_MaxRenderTargets];
    };
}
