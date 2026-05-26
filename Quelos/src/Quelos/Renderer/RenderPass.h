#pragma once

#include "Quelos/Utility/SlotMap.h"
#include "Texture.h"

namespace Quelos {
    enum class ResourceState : uint32_t {
        Unknown = 0,
        Undefined = 1u << 0,
        VertexBuffer = 1u << 1,
        ConstantBuffer = 1u << 2,
        IndexBuffer = 1u << 3,
        RenderTarget = 1u << 4,
        UnorderedAccess = 1u << 5,
        DepthWrite = 1u << 6,
        DepthRead = 1u << 7,
        ShaderResource = 1u << 8,
        StreamOut = 1u << 9,
        IndirectArgument = 1u << 10,
        CopyDest = 1u << 11,
        CopySource = 1u << 12,
        ResolveDest = 1u << 13,
        ResolveSource = 1u << 14,
        InputAttachment = 1u << 15,
        Present = 1u << 16,
        BuildAsRead = 1u << 17,
        BuildAsWrite = 1u << 18,
        RayTracing = 1u << 19,
        Common = 1u << 20,
        ShadingRate = 1u << 21,

        MaxBit = ShadingRate,

        GenericRead = VertexBuffer
                        | ConstantBuffer
                        | IndexBuffer
                        | ShaderResource
                        | IndirectArgument
                        | CopySource
    };

    enum class AttachmentLoadOp : uint8_t {
        Load = 0,
        Clear = 1,
        Discard = 2
    };

    enum class AttachmentStoreOp : uint8_t {
        Store = 0,
        Discard = 1
    };

    struct RenderPassAttachmentSpec {
        ImageFormat Format = ImageFormat::None;
        uint8_t SampleCount = 1;
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Load;
        AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
        AttachmentLoadOp StencilLoadOp = AttachmentLoadOp::Load;
        AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::Store;
        ResourceState InitialState = ResourceState::Unknown;
        ResourceState FinalState = ResourceState::Unknown;
    };

    struct AttachmentReference {
        uint32_t AttachmentIndex = 0;
        ResourceState State = ResourceState::Unknown;
    };

    struct SubPassSpec {
        Span32<const AttachmentReference> RenderTargetAttachments;
        AttachmentReference DepthAttachment;
    };

    struct RenderPassSpec {
        std::string_view Name;
        Span32<const SubPassSpec> SubPasses;
        Span32<const RenderPassAttachmentSpec> Attachments;
    };

    class RenderPass;

    struct RenderPassHandle : Handle<RenderPass> {
        RenderPassHandle() = default;

        RenderPassHandle(const Handle handle) : Handle(handle) {}
    };
}
