#pragma once

#include "VertexBuffer.h"

namespace Quelos {
    struct MeshData {
        MeshData() = default;
        MeshData(std::string name, Vec<Vertex> vertices, Vec<uint16_t> indices) :
            Name(std::move(name)),
            Vertices(std::move(vertices)),
            Indices(std::move(indices))
        {}

        AssetID AssetId;
        std::string Name;

        Vec<Vertex> Vertices;
        Vec<uint16_t> Indices;
    };
}
