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
        uint32_t count = 0;
        m_World.each([viewId, &count](const TransformComponent& transform, const MeshComponent& mesh) {
            Renderer::SubmitMesh(viewId, mesh, transform);
            QS_CORE_INFO("{}, {}, {}", transform.Position.x, transform.Position.y, transform.Position.z);
            count++;
        });

        QS_CORE_INFO("{}", count);
    }

    void Scene::EndRender() const {
    }

    Entity Scene::CreateEntity(const std::string& entityName) {
        const EntityID guid = EntityID::Generate();
        return CreateEntity(guid, entityName);
    }

    Entity Scene::CreateEntity(const EntityID& guid, const std::string& entityName) {
        const Entity entity(m_World.entity().set_name(entityName.c_str()).set(RuntimeTag(guid)));
        m_EntityMap[guid] = entity;
        return entity;
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
