#pragma once

#include "Quelos/Math/Math.h"

#include "Quelos/Serialization/BinaryWriter.h"
#include "Quelos/Serialization/Serializer.h"

#include "Quelos/AssetManager/Assets/Mesh.h"

#include "Quelos/Renderer/Material.h"
#include "Quelos/Renderer/SceneCamera.h"
#include "Quelos/Renderer/Shader.h"
#include "Quelos/Renderer/Texture.h"

#include "Quelos/Renderer/Color.h"

namespace Quelos {
    using EntityID = GUID64;

    class IndexBuffer;
    class VertexBuffer;

    class Scene;

    struct QS_API SceneRoot {
        SceneRoot() = default;

        explicit SceneRoot(Scene* scene) : m_Scene(scene) {}
        [[nodiscard]] Ref<Scene> GetScene() const;
    private:
        Scene* m_Scene = nullptr;
    };

    struct QS_API ActorTag { };

    struct QS_API ChildOrder {
        uint64_t Value = 0;
    };

    struct QS_API LocalTransform {
        float3 Position;
        quaternion Rotation = quaternion::identity();
        float3 Scale = float3(1);

        template <typename TArchive>
        static void Serialize(TArchive& archive, LocalTransform& data) {
            archive.Field("position", data.Position);
            archive.Field("rotation", data.Rotation);
            archive.Field("scale", data.Scale);
        }
    };

    struct QS_API WorldTransform {
        float4x4 Value;
    };

    struct QS_API CameraComponent {
        SceneCamera Camera;

        template <typename TArchive>
        static void Serialize(TArchive& archive, CameraComponent& data) {
            SceneCamera::ProjectionType projectionType = data.Camera.GetProjectionType();
            float persFov = data.Camera.GetPerspectiveVerticalFOV();
            float persNear = data.Camera.GetPerspectiveNearClip();
            float persFar = data.Camera.GetPerspectiveFarClip();
            float orthoSize = data.Camera.GetOrthographicSize();
            float orthoNear = data.Camera.GetOrthographicNearClip();
            float orthoFar = data.Camera.GetOrthographicFarClip();

            archive.Field("projection", projectionType);
            archive.Field("lens.fov", persFov);
            archive.Field("lens.near", persNear);
            archive.Field("lens.far", persFar);

            archive.Field("ortho.size", orthoSize);
            archive.Field("ortho.near", orthoNear);
            archive.Field("ortho.far", orthoFar);

            if constexpr (TArchive::IsLoading) {
                data.Camera.SetProjectionType(projectionType);
                data.Camera.SetPerspectiveVerticalFOV(persFov);
                data.Camera.SetPerspectiveNearClip(persNear);
                data.Camera.SetPerspectiveFarClip(persFar);
                data.Camera.SetOrthographicSize(orthoSize);
                data.Camera.SetOrthographicNearClip(orthoNear);
                data.Camera.SetOrthographicFarClip(orthoFar);

                data.Camera.RecalculateProjection();
            }
        }
    };

    struct QS_API MeshRenderer {
        AssetRef<Mesh> MeshData;
        AssetRef<GraphicsShader> ShaderData;
        Color Color = Color::White();
        //Ref<Material> MaterialData = CreateRef<Material>();
        ShaderResourceBindingHandle ShaderResourceBindingHandle;

        template <typename TArchive>
        static void Serialize(TArchive& archive, MeshRenderer& data) {
            archive.Field("mesh", data.MeshData);
            archive.Field("shader", data.ShaderData);
            archive.Field("color", data.Color);

            if constexpr (TArchive::IsLoading) {
                //data.MaterialData = CreateRef<Material>();
            }
        }
    };

    struct QS_API SpriteRenderer {
        AssetRef<Texture2D> Texture;

        template <typename TArchive>
        static void Serialize(TArchive& archive, SpriteRenderer& data) {
            archive.Field("texture", data.Texture);
        }
    };

    template <typename... Component>
    struct ComponentGroup { };

    using AllComponents = ComponentGroup<LocalTransform, CameraComponent, MeshRenderer, SpriteRenderer>;
}
