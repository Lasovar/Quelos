//
// Created by lasovar on 5/23/26.
//

#pragma once
#include "Renderer.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    // TODO: reference counting
    // For now this is just a scoped resource that will be destroyed at the end of the scope
    template <typename T>
    struct ScopedRenderResource {
        ScopedRenderResource() = default;
        ScopedRenderResource(Handle<T> handle) : Handle(handle) {}

        ~ScopedRenderResource() {
            Renderer::Destroy(Handle);
        }

        operator Handle<T>() const { return Handle; }

        Handle<T> Handle;
    };
}
