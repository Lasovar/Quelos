#include <qspch.h>
#include "Scene.h"
#include <flecs.h>
#include <utility>

#include "Components.h"

#include "Quelos/Core/Events/WindowEvents.h"
#include "Quelos/Math/Math.h"
#include "Quelos/Renderer/Camera.h"

#include "Quelos/Renderer/FrameBuffer.h"
#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    class WindowResizeEvent;

    AssetType Scene::s_AssetType = Quelos::GetAssetType<Scene>();

    Scene::Scene(std::string name)
        : m_Name(std::move(name)
    ) {
        m_ComponentRegistry.RegisterBuiltinTypes(m_World);

        m_SceneRoot = m_World.entity<SceneRoot>("Root")
                             .add<EntityID>()
                             .add(flecs::OrderedChildren);

        m_SceneRoot.set(SceneRoot { this });

        m_World.observer<LocalTransform>()
               .event(flecs::OnAdd)
               .each([](const flecs::entity e, const LocalTransform&) {
                   e.ensure<WorldTransform>();
               });

        m_CameraQuery = m_World.query<const WorldTransform&, const CameraComponent&>();
        m_RenderingQuery = m_World.query<const WorldTransform&, const MeshComponent&>();

        m_TransformUpdate = m_World.entity("TransformUpdate")
                                   .add(flecs::Phase)
                                   .depends_on(flecs::OnUpdate);

        m_TransformChildUpdate = m_World.entity("TransformChildUpdate")
                                        .add(flecs::Phase)
                                        .depends_on(m_TransformUpdate);

        m_World.system<const LocalTransform&, WorldTransform&>()
               .with(flecs::ChildOf, m_SceneRoot)
               .kind(m_TransformUpdate)
               .each([](const LocalTransform& local, WorldTransform& world) {
                   world.Value = Math::SRTMatrix(local);
               });

        m_World.system<const LocalTransform&, WorldTransform&>()
               .without(flecs::ChildOf, flecs::Wildcard)
               .kind(m_TransformUpdate)
               .each([](const LocalTransform& local, WorldTransform& world) {
                   world.Value = Math::SRTMatrix(local);
               });

        m_World.system<const LocalTransform&, WorldTransform&, const WorldTransform&>()
               .without(flecs::ChildOf, m_SceneRoot)
               .with(flecs::ChildOf, flecs::Wildcard)
               .term_at(2).cascade()
               .kind(m_TransformChildUpdate)
               .each([](
                   const LocalTransform& local,
                   WorldTransform& world,
                   const WorldTransform& parentWorld) {
                       world.Value = parentWorld.Value * Math::SRTMatrix(local);
                   });
    }

    void Scene::Tick(const float deltaTime) const {
        QS_PROFILE_SCOPED_N("Scene::Tick");
        if (!m_World.progress(deltaTime)) {
            QS_CORE_ERROR_TAG("Scene", "Failed to progress world");
        }
    }

    void Scene::StartRender(const Ref<FrameBuffer>& frameBuffer) {
        if (m_CameraQuery.count() < 1) {
            return;
        }

        const flecs::entity cameraEntity = m_CameraQuery.first();
        const auto& transform = cameraEntity.get<WorldTransform>();
        const auto& camera = cameraEntity.get<CameraComponent>().Camera;

        Renderer::StartSceneRender(
            frameBuffer,
            transform,
            camera.GetProjection()
        );

        m_SceneRenderStarted = true;
    }

    void Scene::Render(uint32_t viewId) const {
        if (!m_SceneRenderStarted) {
            return;
        }

        m_RenderingQuery.each([viewId](const WorldTransform& transform, const MeshComponent& mesh) {
            if (!mesh.MeshData) {
                return;
            }

            Renderer::SubmitMesh(viewId, mesh, transform);
        });
    }

    void Scene::EndRender() {
        m_SceneRenderStarted = false;
    }

    Actor Scene::CreateActor(const std::string_view entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateActor(guid, entityName);
    }

    Actor Scene::CreateActor(const EntityID& guid, const std::string_view entityName) {
        auto it = m_ActorsMap.find(guid);
        if (it != m_ActorsMap.end()) {
            return it->second;
        }

        const Actor actor = { CreateEntity(guid, entityName), guid };
        actor.Add<ActorTag>();
        SetActorParentToRoot(actor);
        m_ActorsMap[guid] = actor;
        return actor;
    }

    Actor Scene::CreateActor(const EntityID& guid, const std::string_view entityName, const EntityID parent) {
        const Actor actor = CreateActor(guid, entityName);
        if (parent) {
            auto it = m_EntitiesMap.find(parent);
            if (it != m_EntitiesMap.end()) {
                actor.SetParent({ it->second, parent });
            } else {
                SetActorParentToRoot(actor);
            }
        }

        return actor;
    }

    Entity Scene::CreateEntity(const std::string_view entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateEntity(guid, entityName);
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string_view entityName) {
        const flecs::entity entityId = m_World.entity()
                                              .set(guid)
                                              .add<ChildOrder>()
                                              .add(flecs::OrderedChildren);

        const Entity entity(entityId);
        entity.SetName(entityName);
        m_EntitiesMap[guid] = entity;
        return entity;
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string_view entityName, EntityID parent) {
        const Entity entity = CreateEntity(guid, entityName);
        if (parent) {
            const auto it = m_EntitiesMap.find(parent);
            if (it != m_EntitiesMap.end()) {
                entity.SetParent(it->second);
            } else {
                entity.RemoveParent();
            }
        } else {
            entity.RemoveParent();
        }

        return entity;
    }

    void Scene::DestroyEntity(const EntityID entityId) {
        const auto entity = m_ActorsMap[entityId];
        entity.Destroy();
        m_ActorsMap.erase(entityId);
    }

    void Scene::SetActorParentToRoot(const Actor& actor) const {
        actor.GetInternalID().child_of(m_SceneRoot);
    }

    void Scene::OnViewportResized(glm::vec2 viewportSize) const {
        m_World.each([viewportSize](CameraComponent& camera) {
            camera.Camera.SetViewportSize(viewportSize.x, viewportSize.y);
        });
    }

    Ref<Scene> Scene::Copy(const Ref<Scene>& scene) {
        Ref<Scene> sceneCopy = CreateRef<Scene>();

        auto x = sceneCopy->m_World.to_json();
        sceneCopy->m_Name = scene->m_Name;

        return sceneCopy;
    }
}
