#include "qspch.h"
#include "RendererContext.h"

#include "Quelos/Platform/bgfx/bgfxRendererContext.h"

namespace Quelos {
    Ref<RendererContext> RendererContext::Create() {
        return CreateRef<bgfxRendererContext>();
    }
}
