#include <qspch.h>
#include "Scene.h"

#include <flecs.h>

#include <utility>

#include "Components.h"
#include "Quelos/Renderer/FrameBuffer.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
    Scene::Scene(std::string  name)
        : m_Name(std::move(name))
    {
    }

    void Scene::Tick(const float deltaTime) const {
        m_World.progress(deltaTime);
    }

    void Scene::Render(uint32_t viewId, const Ref<FrameBuffer>& frameBuffer) const {
        m_World.each([this, viewId, frameBuffer](const TransformComponent& renderCameraTransform, const CameraComponent& renderCamera) {
            Renderer::StartSceneRender(viewId, frameBuffer, renderCamera, renderCameraTransform);

            m_World.each([viewId](const TransformComponent& transform, const MeshComponent& mesh) {
                Renderer::SubmitMesh(viewId, mesh, transform);
            });
        });
    }

    Entity Scene::CreateEntity(const std::string& entityName) {
        const GUID guid = GUID::Generate();
        return CreateEntity(guid, entityName);
    }

    Entity Scene::CreateEntity(const GUID& guid, const std::string& entityName) {
        const auto id = m_World.make_alive(guid.Hash64()).set_name(entityName.c_str());
        const Entity entity(id);
        m_EntityMap[guid] = entity;
        QS_CORE_INFO("{}", std::format("Created entity '{}' with GUID {}", entityName, guid.ToString()));
        return entity;
    }
}
