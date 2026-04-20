#include "qspch.h"
#include "Asset.h"

namespace Quelos {
    const AssetType AssetType::Invalid = { 0, "" };

    AssetType GetAssetType(std::string name) {
        uint64_t id = Hash::Fnv1a64(name);
        return { id, std::move(name) };
    }
}
