#pragma once

#include <Quelos/Core/Layer.h>
#include <Quelos/Scenes/Scene.h>
#include <Quelos/Core/Ref.h>

namespace Quelos {
	class EditorLayer : public Layer {
	public:
		EditorLayer();

		void OnAttach() override;

		void Tick(float deltaTime) override;
		void ImGuiRender() override;
	private:
		Scene m_DefaultScene;
	};
}

