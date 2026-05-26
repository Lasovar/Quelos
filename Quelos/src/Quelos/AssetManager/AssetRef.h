#pragma once

#include "AssetManagerBase.h"
#include "AssetPool.h"
#include "Quelos/Project/Project.h"

namespace Quelos {
    template <typename T>
    requires (std::is_base_of_v<Asset, T>)
    class AssetRef {
    public:
        AssetRef() = default;
        explicit AssetRef(const AssetID assetId) {
            if (!assetId.IsValid()) [[unlikely]] {
                return;
            }

            m_Handle = Project::GetAssetManager()->Acquire(assetId);
            Inc();
        }

        AssetRef(const AssetHandle<T> assetHandle) {
            m_Handle = assetHandle;
            Inc();
        }

        AssetRef(const AssetRef& other)
            : m_Handle(other.m_Handle)
        {
            Inc();
        }

        AssetRef& operator=(const AssetRef& other) {
            if (this == &other) {
                return *this;
            }

            Dec();

            m_Handle = other.m_Handle;

            Inc();
            return *this;
        }

        AssetRef(AssetRef&& other) noexcept {
            m_Handle = other.m_Handle;
            other.m_Handle = AssetHandle<T>();
        }

        AssetRef& operator=(AssetRef&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            Dec();

            m_Handle = other.m_Handle;
            other.m_Handle = AssetHandle<T>();

            return *this;
        }

        ~AssetRef() {
            Dec();
        }

        AssetHandle<T> GetAssetHandle() const { return m_Handle; }

        void Reset() {
            Dec();
            m_Handle = AssetHandle<T>();
        }

        T* TryGet() const {
            return static_cast<T*>(Project::GetAssetManager()->TryGet(m_Handle));
        }

        T* operator->() const {
            return TryGet();
        }

        T& Get() const {
            return *static_cast<T*>(Project::GetAssetManager()->TryGet(m_Handle));
        }

        // Whether the asset handle is valid or not
        [[nodiscard]] bool IsValid() const {
            return m_Handle.IsValid();
        }

        // Whether the asset is loaded or not
        [[nodiscard]] bool IsAlive() const {
            return IsValid() && Project::GetAssetManager()->IsAlive(m_Handle);
        }

        operator bool() const {
            return IsAlive();
        }

        [[nodiscard]] AssetID GetAssetID() const {
            T* asset = TryGet();
            return asset ? asset->GetAssetID() : AssetID();
        }

    private:
        void Inc() {
            if (!m_Handle.IsValid()) [[unlikely]] {
                return;
            }

            Project::GetAssetManager()->IncRef(m_Handle);
        }

        void Dec() {
            if (!m_Handle.IsValid()) [[unlikely]] {
                return;
            }

            Project::GetAssetManager()->DecRef(m_Handle);
        }

    private:
        AssetHandle<T> m_Handle;
    };
}
