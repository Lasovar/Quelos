//
// Created by lasovar on 5/23/26.
//

#pragma once
#include "Renderer.h"
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
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

    template <typename T>
    class ResourceRef {
    public:
        ResourceRef() = default;
        ResourceRef(const Handle<T> renderResourceHandle) {
            m_Handle = renderResourceHandle;
            Inc();
        }

        ResourceRef(const ResourceRef& other)
            : m_Handle(other.m_Handle)
        {
            Inc();
        }

        ResourceRef& operator=(const ResourceRef& other) {
            if (this == &other) {
                return *this;
            }

            Dec();

            m_Handle = other.m_Handle;

            Inc();
            return *this;
        }

        ResourceRef(ResourceRef&& other) noexcept {
            m_Handle = other.m_Handle;
            other.m_Handle = Handle<T>();
        }

        ResourceRef& operator=(ResourceRef&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            Dec();

            m_Handle = other.m_Handle;
            other.m_Handle = Handle<T>();

            return *this;
        }

        ~ResourceRef() {
            Dec();
        }

        Handle<T> GetHandle() const { return m_Handle; }
        operator Handle<T>() const { return m_Handle; }

        void Reset() {
            m_Handle = Handle<T>();
        }

        // Whether the asset handle is valid or not
        bool IsValid() const {
            return m_Handle.IsValid();
        }

    private:
        void Inc() {
            if (!IsValid()) {
                return;
            }

            Renderer::IncRef(m_Handle);
        }

        void Dec() {
            if (!IsValid()) {
                return;
            }

            Renderer::DecRef(m_Handle);
        }

    private:
        Handle<T> m_Handle;
    };
}
