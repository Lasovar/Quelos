#include "qspch.h"
#include "Asset.h"

namespace Quelos {
    const AssetType AssetType::Invalid = { 0, "" };

    AssetType GetAssetType(std::string name) {
        uint32_t id = Hash::Fnv1a32(name);
        return { id, std::move(name) };
    }
}
