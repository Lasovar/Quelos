#pragma once

#include "Quelos/Core/GUID.h"
#include "Quelos/Core/Ref.h"
#include "Quelos/Utility/Hash.h"

namespace Quelos {
    using AssetHandle = GUID128;

    struct QS_API AssetType {
        AssetType() = default;
        AssetType(const uint64_t value, std::string name) : m_Value(value), m_Name(std::move(name)) {}

        [[nodiscard]] const std::string& GetName() const { return m_Name; }

        operator uint64_t() const { return m_Value; }
        bool operator==(const AssetType& other) const { return m_Value == other.m_Value; }
        bool operator!=(const AssetType& other) const { return m_Value != other.m_Value; }

    public:
        static const AssetType Invalid;
    private:
        uint64_t m_Value = 0;
        std::string m_Name;
    };

    class QS_API Asset : public RefCounted<Asset> {
    public:
        virtual ~Asset() = default;
        AssetHandle GetAssetHandle() const { return Handle; }
        void SetAssetHandle(const AssetHandle handle) { Handle = handle; }

        static const AssetType& GetStaticType() { return AssetType::Invalid; }
        virtual const AssetType& GetAssetType() const = 0;
    protected:
        AssetHandle Handle;
    };

    template <typename  T>
    requires (std::is_base_of_v<Asset, T> && !std::same_as<std::remove_cvref_t<T>, Asset>)
    constexpr AssetType GetAssetType() {
        std::string assetTypeName = TypeNameDisplay<T>();
        return { Hash::Fnv1a64(assetTypeName), assetTypeName };
    }

    constexpr AssetType GetAssetType(std::string name) {
        return { Hash::Fnv1a64(name), std::move(name) };
    }
}


template<>
struct std::hash<Quelos::AssetType> {
    std::size_t operator()(const Quelos::AssetType& assetType) const noexcept {
        return assetType;
    }
};
