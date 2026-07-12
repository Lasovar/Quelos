#pragma once

#include "VertexBuffer.h"
#include "Quelos/Utility/Vec.hpp"

namespace Quelos {
    struct MeshData {
        MeshData() = default;

        AssetID AssetId;
        std::string Name;

        Vec<Vertex> Vertices{Allocator::Persistent};
        Vec<uint16_t> Indices{Allocator::Persistent};
    };
}
