#pragma once
#include <string>
#include "Event.h"
#include <Quelos/Core/Ref.h>

namespace Quelos {
	class Layer : public RefCounted {
	public:
		Layer(const std::string& DebugName = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void Tick(float deltaTime) {}
		virtual void ImGuiRender() {}

		virtual void OnEvent(Event& event) {}

		std::string GetName() const { return DebugName; }
	private:
		std::string DebugName;
	};
}

