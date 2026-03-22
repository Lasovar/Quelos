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

    HashMap<ecs_world_t*, Scene*> Scene::s_WorldToScene{};

    Scene::Scene(std::string name)
        : m_Name(std::move(name)) {
        m_ComponentRegistry.RegisterBuiltinTypes(m_World);
        s_WorldToScene[m_World.c_ptr()] = this;

        m_SceneRoot = m_World.entity<SceneRootTag>("Root")
                             .add<ActorTag>()
                             .add(flecs::OrderedChildren);

        if (m_World.singleton<SceneRootTag>().has(flecs::OrderedChildren)) {
            QS_CORE_INFO("Has Ordered children");
        }

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
        if (!m_World.progress(deltaTime)) {
            QS_CORE_ERROR_TAG("Scene", "Failed to progress world");
        }
    }

    void Scene::StartRender(const Ref<FrameBuffer>& frameBuffer) const {
        const auto query = m_World.query<LocalTransform, CameraComponent>();
        const Entity cameraEntity = query.first();
        auto transform = cameraEntity.Get<WorldTransform>();
        auto camera = cameraEntity.Get<CameraComponent>().Camera;
        Renderer::StartSceneRender(
            frameBuffer,
            transform,
            camera.GetProjection()
        );
    }

    void Scene::Render(uint32_t viewId) const {
        m_World.each([viewId](const WorldTransform& transform, const MeshComponent& mesh) {
            Renderer::SubmitMesh(viewId, mesh, transform);
        });
    }

    void Scene::EndRender() const {
    }

    Actor Scene::CreateActor(const std::string_view entityName) {
        const ActorID guid = ActorID::Generate();
        return CreateActor(guid, entityName);
    }

    Actor Scene::CreateActor(const ActorID& guid, const std::string_view entityName) {
        const flecs::entity entityId = m_World.entity()
                                              .set(ActorTag(guid))
                                              .add<ChildOrder>()
                                              .add(flecs::OrderedChildren);

        const Actor actor(entityId, guid);
        actor.SetName(entityName);
        actor.OrderBack(Actor(m_SceneRoot, ActorID()));

        m_ActorsMap[guid] = actor;
        return actor;
    }

    void Scene::DestroyEntity(const ActorID entityId) {
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
