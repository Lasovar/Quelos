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

    Scene::Scene(std::string  name)
        : m_Name(std::move(name))
    {
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

    void Scene::EndRender() const { }

    Entity Scene::CreateEntity(const std::string& entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateEntity(guid, entityName);
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string& entityName) {
        const Entity entity(m_World.make_alive(guid).set_name(entityName.c_str()));
        m_EntityMap[guid] = entity;
        return entity;
    }

    void Scene::OnViewportResized(glm::vec2 viewportSize) const {
        m_World.each([viewportSize](CameraComponent& camera) {
            camera.Camera.SetViewportSize(viewportSize.x, viewportSize.y);
        });
    }
}
