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

        m_World.system<const LocalTransform, WorldTransform>()
               .without(flecs::ChildOf, flecs::Wildcard)
               .kind(m_TransformUpdate)
               .each([](const LocalTransform& local, WorldTransform& world) {
                   world.Value = Math::SRTMatrix(local);
               });

        m_World.system<const LocalTransform, WorldTransform, const WorldTransform>()
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

    Entity Scene::CreateActor(const std::string_view entityName) {
        const ActorID guid = ActorID::Generate();
        return CreateActor(guid, entityName);
    }

    inline void SetNameFromView(const flecs::entity e, const std::string_view view) {
        constexpr size_t MaxStack = 32;

        if (view.size() < MaxStack) {
            static char buffer[MaxStack];
            std::memcpy(buffer, view.data(), view.size());
            buffer[view.size()] = '\0';
            e.set_name(buffer);
        }
        else {
            const std::string temp(view);
            e.set_name(temp.c_str());
        }
    }

    Entity Scene::CreateActor(const ActorID& guid, const std::string_view entityName) {
        const flecs::entity entityId = m_World.entity().set(ActorTag(guid));
        SetNameFromView(entityId, entityName);
        const Entity entity(entityId);
        m_EntityMap[guid] = entity;
        return entity;
    }

    void Scene::DestroyEntity(const ActorID entityId) {
        const auto entity = m_EntityMap[entityId];
        entity.Destroy();
        m_EntityMap.erase(entityId);
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
