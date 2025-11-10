#include <qspch.h>
#include "Scene.h"

#include <flecs.h>

#include "Components.h"
#include <iostream>

namespace Quelos {
	Scene::Scene() {
	}

	void Scene::Tick(float deltaTime) {
		//m_World.each([](flecs::entity e, TransformComponent& t) {});
	}

	Entity Scene::CreateEntity(const std::string& entityName) {
		return Entity(m_World.entity(entityName.c_str()));
	}

}

