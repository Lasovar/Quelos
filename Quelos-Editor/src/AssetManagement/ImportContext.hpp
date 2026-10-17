//
// Created by lasovar on 10/13/26.
//

#pragma once
#include "Quelos/AssetManager/AssetMetadata.h"
#include "Quelos/AssetManager/AssetRegistry.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Core/DataTypes.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    class ImportContext {
    public:
        ImportContext(const AssetMetadata& asset, AssetRegistry& registry, std::pmr::memory_resource* memoryResource)
            : m_Asset(asset), m_Registry(registry),
              m_Dependencies(memoryResource), m_Artifacts(memoryResource), m_SubAssets(memoryResource) {}

        [[nodiscard]] const AssetMetadata& GetAssetMetadata() const { return m_Asset; }
        [[nodiscard]] Span<const String> GetArtifacts() const { return m_Artifacts; }
        [[nodiscard]] Span<const AssetID> GetDependencies() const { return m_Dependencies; }
        [[nodiscard]] Span<const AssetMetadata> GetSubAssets() const { return m_SubAssets; }

        /// Add another asset as a dependency
        /// If the dependency is reimported it will also reimport this asset
        void DependOn(const AssetID& id);
        void DependOn(std::string_view projectRelativePath);

        /// Writes an artifact under the library folder
        /// The artifact will be written in `Library/Artifacts/{AssetID[firstTwoChars]/{AssetID}/{name}
        /// To retrieve the asset, use the same `name` used for writing it
        Expected<void, String> WriteArtifact(std::string_view name, BufferView data);

        /// Sub-assets this import produced
        /// `stableName` is a virtual path appended to the end of the path
        /// if `stableName` is `Meshes/Cube` the path stored will be
        /// `Assets/Models/Cube.fbx/Meshes/Cube`
        /// TODO: Ability to retrieve asset through virtual path
        AssetID AddSubAsset(std::string_view stableName, const AssetType& type);
        AssetID AddSubAsset(std::string_view stableName, AssetID subAssetId, const AssetType& type);

    private:
        const AssetMetadata& m_Asset;
        AssetRegistry& m_Registry;
        Vec<AssetID> m_Dependencies;
        /// Generated artifacts path relative to the `Library` directory
        Vec<String> m_Artifacts;
        Vec<AssetMetadata> m_SubAssets;
    };
}
