#include <qspch.h>
#include "Scene.h"
#include <flecs.h>
#include <utility>

#include "Components.h"

#include "Quelos/Core/Events/WindowEvents.h"
#include "Quelos/Math/Math.h"
#include "Quelos/Renderer/Camera.h"
#include "Quelos/ImGui/ImGuiUI.h"

#include "Quelos/Renderer/FrameBuffer.h"
#include "Quelos/Renderer/Renderer.h"
#include "Quelos/Core/Profiling.h"

namespace Quelos {
    class WindowResizeEvent;

    Scene::Scene(std::string name)
        : m_Name(std::move(name)), m_SceneRenderer(m_World)
    {
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
                   world.Value = mathExt::srt(local.Scale, local.Rotation, local.Position);
               });

        m_World.system<const LocalTransform&, WorldTransform&>()
               .without(flecs::ChildOf, flecs::Wildcard)
               .kind(m_TransformUpdate)
               .each([](const LocalTransform& local, WorldTransform& world) {
                   world.Value = mathExt::srt(local.Scale, local.Rotation, local.Position);
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
                       world.Value = math::mul(parentWorld.Value, mathExt::srt(local.Scale, local.Rotation, local.Position));
                   });

        m_CameraQuery = m_World.query<const WorldTransform&, const CameraComponent&>();
    }

    void Scene::Tick(const float deltaTime) const {
        QS_PROFILE_SCOPED_N("Scene::Tick");
        if (!m_World.progress(deltaTime)) {
            QS_CORE_ERROR_TAG("Scene", "Failed to progress world");
        }
    }

    void Scene::StartRender(const BeginRenderPassAttribs& beginRenderPassAttribs) {
        if (m_CameraQuery.count() < 1) {
            return;
        }

        const flecs::entity cameraEntity = m_CameraQuery.first();
        const auto& transform = cameraEntity.get<WorldTransform>();
        const auto& camera = cameraEntity.get<CameraComponent>().Camera;

        StartRender(mathExt::view(transform.Value), camera.GetProjection(), beginRenderPassAttribs);
    }

    void Scene::StartRender(float4x4 view, float4x4 projection, const BeginRenderPassAttribs& beginRenderPassAttribs) {
        m_SceneRenderer.Begin(beginRenderPassAttribs, math::mul(view, projection));
        m_SceneRenderStarted = true;
    }

    void Scene::Render() {
        if (!m_SceneRenderStarted) {
            return;
        }

        m_SceneRenderer.Render();
    }

    void Scene::EndRender() {
        if (m_SceneRenderStarted) {
            m_SceneRenderer.End();
        }

        m_SceneRenderStarted = false;
    }

    Actor Scene::CreateActor(const std::string_view entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateActor(guid, entityName);
    }

    Actor Scene::CreateActor(const EntityID& guid, const std::string_view entityName) {
        if (const auto it = m_ActorsMap.find(guid); it != m_ActorsMap.end()) {
            return it->second;
        }

        const Actor actor = { CreateEntity(guid, entityName, EntityID()), guid };
        actor.Add<ActorTag>();
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
        entity.GetInternalID().set_name(guid.ToString().c_str());
        entity.SetName(entityName);
        m_EntitiesMap[guid] = entity;
        return entity;
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string_view entityName, EntityID parent) {
        const Entity entity = CreateEntity(guid, entityName);
        if (parent) {
            if (const auto it = m_EntitiesMap.find(parent); it != m_EntitiesMap.end()) {
                entity.SetParent(it->second);
            } else {
                entity.RemoveParent();
            }
        } else {
            entity.GetInternalID().child_of(m_SceneRoot);
        }

        return entity;
    }

    void Scene::DestroyEntity(const EntityID entityId) {
        const auto it = m_EntitiesMap.find(entityId);
        if (it == m_EntitiesMap.end()) {
            return;
        }

        it->second.Destroy();
        m_EntitiesMap.erase(entityId);
        m_ActorsMap.erase(entityId);
    }

    void Scene::SetActorParentToRoot(const Actor& actor) const {
        actor.GetInternalID().child_of(m_SceneRoot);
    }

    void Scene::OnViewportResized(float2 viewportSize) const {
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
