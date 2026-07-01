#pragma once

#include <cstdint>

namespace Quelos {
    /// Fill mode

    /// [D3D11_FILL_MODE]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476131(v=vs.85).aspx
    /// [D3D12_FILL_MODE]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770366(v=vs.85).aspx
    /// This enumeration determines the fill mode to use when rendering triangles and mirrors the
    /// [D3D11_FILL_MODE][]/[D3D12_FILL_MODE][] enum. It is used by RasterizerStateDesc structure to define the fill mode.
    enum class FillMode : int8_t {
        /// Undefined fill mode.
        Undefined = 0,

        /// Rasterize triangles using wireframe fill. \n
        /// Direct3D counterpart: D3D11_FILL_WIREFRAME/D3D12_FILL_MODE_WIREFRAME. OpenGL counterpart: GL_LINE.
        Wireframe,

        /// Rasterize triangles using solid fill. \n
        /// Direct3D counterpart: D3D11_FILL_SOLID/D3D12_FILL_MODE_SOLID. OpenGL counterpart: GL_FILL.
        Solid,

        /// Helper value that stores the total number of fill modes in the enumeration.
        Count
    };

    /// Cull mode

    /// [D3D11_CULL_MODE]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476108(v=vs.85).aspx
    /// [D3D12_CULL_MODE]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770354(v=vs.85).aspx
    /// This enumeration defines which triangles are not drawn during the rasterization and mirrors
    /// [D3D11_CULL_MODE][]/[D3D12_CULL_MODE][] enum. It is used by RasterizerStateDesc structure to define the polygon cull mode.
    enum class CullMode : int8_t {
        /// Undefined cull mode.
        Undefined = 0,

        /// Draw all triangles. \n
        /// Direct3D counterpart: D3D11_CULL_NONE/D3D12_CULL_MODE_NONE. OpenGL counterpart: glDisable( GL_CULL_FACE ).
        None,

        /// Do not draw triangles that are front-facing. Front- and back-facing triangles are determined
        /// by the RasterizerStateDesc::FrontCounterClockwise member. \n
        /// Direct3D counterpart: D3D11_CULL_FRONT/D3D12_CULL_MODE_FRONT. OpenGL counterpart: GL_FRONT.
        Front,

        /// Do not draw triangles that are back-facing. Front- and back-facing triangles are determined
        /// by the RasterizerStateDesc::FrontCounterClockwise member. \n
        /// Direct3D counterpart: D3D11_CULL_BACK/D3D12_CULL_MODE_BACK. OpenGL counterpart: GL_BACK.
        Back,

        /// Helper value that stores the total number of cull modes in the enumeration.
        Count
    };

    struct RasterizerStateSpec {
        /// Determines triangle fill mode, see Diligent::FILL_MODE for details.

        /// Default value: Diligent::FILL_MODE_SOLID.
        FillMode FillMode = FillMode::Solid;

        /// Determines triangle cull mode, see Diligent::CULL_MODE for details.

        /// Default value: Diligent::CULL_MODE_BACK.
        CullMode CullMode = CullMode::Back;

        /// Determines if a triangle is front- or back-facing.

        /// If this parameter is True, a triangle will be considered front-facing if
        /// its vertices are counter-clockwise
        /// on the render target and considered back-facing if they are clockwise.
        /// If this parameter is False, the opposite is true.
        /// Default value: False.
        bool FrontCounterClockwise = false;

        /// Enable clipping against near and far clip planes.

        /// Default value: True.
        ///
        /// By default polygon faces are clipped against the near and far planes of the view
        /// frustum. If depth clipping is disabled, the depth of the fragments that would be
        /// clipped is clamped to the near/far plane instead of discarding them.
        ///
        /// To check if the device supports depth clamping, use the DepthClamp device feature.
        /// If it is not supported, the value of this member must be True.
        bool DepthClipEnable = true;

        /// Enable scissor-rectangle culling. All pixels outside an active scissor rectangle are culled.

        /// Default value: False.
        bool ScissorEnabled = false;

        /// Specifies whether to enable line antialiasing.

        /// Default value: False.
        bool AntialiasedLineEnabled = false;

        /// Constant value added to the depth of a given pixel.

        /// Default value: 0.
        int32_t DepthBias = 0;

        /// Maximum depth bias of a pixel.

        /// \warning Depth bias clamp is not available in OpenGL
        ///
        /// Default value: 0.
        float DepthBiasClamp = 0.0f;

        /// Scalar that scales the given pixel's slope before adding to the pixel's depth.

        /// Default value: 0.
        float SlopeScaledDepthBias = 0.0f;
    };
}
