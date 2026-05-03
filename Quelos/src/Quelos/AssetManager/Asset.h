#pragma once

#include "Quelos/Core/GUID.h"
#include "Quelos/Utility/Hash.h"

namespace Quelos {
    using AssetID = GUID128;
    using AssetTypeID = uint32_t;

    struct QS_API AssetType {
        AssetType() = default;
        AssetType(const AssetTypeID value, std::string name) : m_Value(value), m_Name(std::move(name)) {}

        [[nodiscard]] const std::string& GetName() const { return m_Name; }

        operator AssetTypeID() const { return m_Value; }
        bool operator==(const AssetType& other) const { return m_Value == other.m_Value; }
        bool operator!=(const AssetType& other) const { return m_Value != other.m_Value; }

    public:
        static const AssetType Invalid;
    private:
        AssetTypeID m_Value = 0;
        std::string m_Name;

        friend class std::hash<AssetType>;
    };

    class QS_API Asset {
    public:
        virtual ~Asset() = default;
        [[nodiscard]] AssetID GetAssetID() const { return m_AssetId; }
        void SetAssetID(const AssetID handle) { m_AssetId = handle; }

        static const AssetType& GetStaticType() { return AssetType::Invalid; }
        [[nodiscard]] virtual const AssetType& GetAssetType() const = 0;
    protected:
        AssetID m_AssetId;
    };

    QS_API AssetType GetAssetType(std::string name);

    template <typename  T>
    requires (std::is_base_of_v<Asset, T> && !std::same_as<std::remove_cvref_t<T>, Asset>)
    constexpr AssetType GetAssetType() {
        std::string assetTypeName = TypeNameDisplay<T>();
        return GetAssetType(std::move(assetTypeName));
    }
}

template<>
struct std::hash<Quelos::AssetType> {
    std::size_t operator()(const Quelos::AssetType& assetType) const noexcept {
        return assetType.m_Value;
    }
};
