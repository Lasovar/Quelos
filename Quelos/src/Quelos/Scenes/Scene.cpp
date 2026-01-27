#include <qspch.h>
#include "Scene.h"

#include <flecs.h>

#include "Components.h"
#include "glm/gtx/quaternion.hpp"

#include "Quelos/Renderer/Renderer.h"
#include "Quelos/Renderer/VertexBuffer.h"
#include "Quelos/Renderer/IndexBuffer.h"
#include "Quelos/Renderer/Shader.h"
#include "Quelos/Renderer/Material.h"

namespace Quelos {
    static std::vector<PosColorVertex> cubeVertices = {
        {-1.0f, 1.0f, 1.0f, 0xff000000},
        {1.0f, 1.0f, 1.0f, 0xff0000ff},
        {-1.0f, -1.0f, 1.0f, 0xff00ff00},
        {1.0f, -1.0f, 1.0f, 0xff00ffff},
        {-1.0f, 1.0f, -1.0f, 0xffff0000},
        {1.0f, 1.0f, -1.0f, 0xffff00ff},
        {-1.0f, -1.0f, -1.0f, 0xffffff00},
        {1.0f, -1.0f, -1.0f, 0xffffffff},
    };

    static const std::vector<uint16_t> cubeTriList = {
        0, 1, 2,
        1, 3, 2,
        4, 6, 5,
        5, 6, 7,
        0, 2, 4,
        4, 2, 6,
        1, 5, 3,
        5, 7, 3,
        0, 4, 1,
        4, 5, 1,
        2, 3, 6,
        6, 3, 7,
    };

    struct CubePlayer {
        float Speed = 2.0f;
        float Timer = 0.0f;
    };

    Scene::Scene() {
        const Entity camera = m_World.entity();
        camera.Set(TransformComponent{glm::vec3(0.0f, 0.0f, -15.0f), glm::quat({0, 0, 0})});
        camera.Set(CameraComponent{60.0f, 0.1f, 1000.0f});

        const Entity cube = m_World.entity();
        cube.Set(TransformComponent{glm::vec3(-2.5f, -2.5f, 0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});

        MeshComponent cubeMesh;
        cubeMesh.VertexBuffer = CreateRef<VertexBuffer>(cubeVertices);
        cubeMesh.IndexBuffer = CreateRef<IndexBuffer>(cubeTriList);
        cubeMesh.Material = CreateRef<Material>(CreateRef<Shader>("vs_cubes.bin", "fs_cubes.bin"));
        cube.Set(cubeMesh);

        cube.Set(CubePlayer());

        const Entity cube2 = m_World.entity();
        cube2.Set(TransformComponent{glm::vec3(2.5f, 2.5f, 0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});
        cube2.Set(cubeMesh);
        cube2.Set(CubePlayer { -2 });

        const Entity cube3 = m_World.entity();
        cube3.Set(TransformComponent{glm::vec3(0), glm::quat({0, 0, 0}), glm::vec3(1.0f)});
        cube3.Set(cubeMesh);
        cube3.Set(CubePlayer { -10 });
    }

    void Scene::Tick(float deltaTime) const {
        m_World.each([deltaTime](TransformComponent& transform, CubePlayer& player) {
            player.Timer += deltaTime;
            transform.Rotation = glm::quat({
                player.Timer * player.Speed,
                player.Timer * player.Speed,
                0
            });
        });
    }

    void Scene::Render() const {
        m_World.each([this](const TransformComponent& renderCameraTransform, const CameraComponent& renderCamera) {
            Renderer::StartSceneRender(renderCamera, renderCameraTransform);

            m_World.each([](const TransformComponent& transform, const MeshComponent& mesh) {
                Renderer::SubmitMesh(mesh, transform);
            });
        });

    }

    Entity Scene::CreateEntity(const std::string& entityName) const {
        return {m_World.entity(entityName.c_str())};
    }
}
