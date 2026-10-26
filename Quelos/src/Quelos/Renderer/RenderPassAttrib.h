#pragma once

#include "Color.h"
#include "RenderPass.h"
#include "FrameBuffer.h"

namespace Quelos {
    struct DepthStencilClearValue {
        float Depth = 1.0f;
        uint8_t Stencil = 0;
    };

    struct ClearValue {
        TextureFormat Format = TextureFormat::None;
        Color Color;
        DepthStencilClearValue DepthStencil;
    };

    struct BeginRenderPassAttribs {
        RenderPassHandle RenderPassHandle;
        FrameBufferHandle FrameBufferHandle;
        Span32<const ClearValue> ClearColors;
    };
}
