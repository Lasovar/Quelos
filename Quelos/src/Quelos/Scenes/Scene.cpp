#include <qspch.h>
#include "Scene.h"

#include <flecs.h>

#include "Components.h"
#include <iostream>

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
	Scene::Scene() {
	}

	void Scene::Tick(float deltaTime) {
		//m_World.each([](flecs::entity e, TransformComponent& t) {});
	}

	void Scene::Render() {
		Renderer::StartSceneRender(CameraComponent { 60.0f, 0.1f, 1000.0f }, TransformComponent());
	}

	Entity Scene::CreateEntity(const std::string& entityName) {
		return Entity(m_World.entity(entityName.c_str()));
	}

}

