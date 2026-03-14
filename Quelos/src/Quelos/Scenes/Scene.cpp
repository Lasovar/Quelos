#include <qspch.h>
#include "Scene.h"
#include <flecs.h>
#include <utility>

#include "Components.h"

#include "Quelos/Core/Events/WindowEvents.h"
#include "Quelos/Renderer/Camera.h"

#include "Quelos/Renderer/FrameBuffer.h"
#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    class WindowResizeEvent;

    Scene::Scene(std::string name)
        : m_Name(std::move(name)
    ) {
        m_ComponentRegistry.RegisterBuiltinTypes(m_World);
    }

    void Scene::Tick(const float deltaTime) const {
        m_World.progress(deltaTime);
    }

    void Scene::StartRender(const Ref<FrameBuffer>& frameBuffer) const {
        const auto query = m_World.query<TransformComponent, CameraComponent>();
        const Entity cameraEntity = query.first();
        auto transform = cameraEntity.Get<TransformComponent>();
        auto camera = cameraEntity.Get<CameraComponent>().Camera;
        Renderer::StartSceneRender(
            frameBuffer,
            transform,
            camera.GetProjection()
        );
    }

    void Scene::Render(uint32_t viewId) const {
        m_World.each([viewId](const TransformComponent& transform, const MeshComponent& mesh) {
            Renderer::SubmitMesh(viewId, mesh, transform);
        });
    }

    void Scene::EndRender() const {
    }

    Entity Scene::CreateEntity(const std::string_view entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateEntity(guid, entityName);
    }

    inline void SetNameFromView(flecs::entity e, std::string_view view) {
        constexpr size_t MaxStack = 32;

        if (view.size() < MaxStack) {
            char buffer[MaxStack];
            std::memcpy(buffer, view.data(), view.size());
            buffer[view.size()] = '\0';
            e.set_name(buffer);
        }
        else {
            const std::string temp(view);
            e.set_name(temp.c_str());
        }
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string_view entityName) {
        const flecs::entity entityId = m_World.entity().set(RuntimeTag(guid));
        SetNameFromView(entityId, entityName);
        const Entity entity(entityId);
        m_EntityMap[guid] = entity;
        return entity;
    }

    void Scene::DestroyEntity(const EntityID entityId) {
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
