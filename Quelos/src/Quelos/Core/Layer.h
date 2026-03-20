#pragma once

#include <string>
#include "Event.h"
#include <Quelos/Core/Ref.h>

namespace Quelos {
	class Layer {
	public:
		explicit Layer(const std::string& debugName = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void Tick(float deltaTime) {}
		virtual void ImGuiRender() {}

		virtual void OnEvent(Event& event) {}

		std::string GetName() const { return m_DebugName; }
	private:
		std::string m_DebugName;
	};
}

